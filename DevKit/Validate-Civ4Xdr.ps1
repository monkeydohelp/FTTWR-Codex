param(
    [Parameter(Mandatory = $true)][string]$XmlPath,
    [Parameter(Mandatory = $true)][string]$SchemaPath,
    [switch]$ContinueOnError
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$script:ValidationErrors = [System.Collections.Generic.List[string]]::new()
$script:ElementTypes = @{}

function Get-ElementChildren {
    param([System.Xml.XmlNode]$Node)
    return @($Node.ChildNodes | Where-Object { $_.NodeType -eq [System.Xml.XmlNodeType]::Element })
}
function Add-ValidationError {
    param([string]$Message)
    $script:ValidationErrors.Add($Message)
}
function Get-UniqueStates {
    param([int[]]$States)
    $seen = @{}
    $unique = [System.Collections.Generic.List[int]]::new()
    foreach ($state in $States) {
        $key = [string]$state
        if (-not $seen.ContainsKey($key)) { $seen[$key] = $true; $unique.Add($state) }
    }
    return $unique.ToArray()
}
function Get-XdrParticles {
    param([System.Xml.XmlNode]$Declaration)
    $particles = [System.Collections.Generic.List[System.Xml.XmlNode]]::new()
    foreach ($child in $Declaration.ChildNodes) {
        if ($child.NodeType -ne [System.Xml.XmlNodeType]::Element) { continue }
        if ($child.LocalName -notin @('element', 'group')) {
            Add-ValidationError "Unsupported XDR declaration '$($child.LocalName)' inside '$($Declaration.GetAttribute('name'))'."
            continue
        }
        $particles.Add($child)
    }
    return $particles.ToArray()
}
function Get-OccurrenceBounds {
    param([System.Xml.XmlNode]$Particle, [int]$Remaining)
    $minimum = 1
    if ($Particle.Attributes['minOccurs']) { $minimum = [int]$Particle.Attributes['minOccurs'].Value }
    $maximum = 1
    if ($Particle.Attributes['maxOccurs']) {
        $maximumText = $Particle.Attributes['maxOccurs'].Value
        if ($maximumText -eq '*') { $maximum = [Math]::Max($Remaining, $minimum) }
        else { $maximum = [int]$maximumText }
    }
    if ($minimum -lt 0 -or $maximum -lt $minimum) {
        Add-ValidationError "Invalid XDR occurrence bounds on '$($Particle.LocalName)' (min=$minimum, max=$maximum)."
    }
    return @($minimum, $maximum)
}
function Match-XdrSingle {
    param([System.Xml.XmlNode]$Particle, [System.Xml.XmlElement[]]$Actual, [int]$Start, [string]$Path)
    if ($Particle.LocalName -eq 'element') {
        $expectedType = $Particle.GetAttribute('type')
        if ($Start -ge $Actual.Count -or $Actual[$Start].LocalName -ne $expectedType) { return @() }
        $childPath = '{0}/{1}[{2}]' -f $Path, $expectedType, ($Start + 1)
        Test-XdrElement -Element $Actual[$Start] -TypeName $expectedType -Path $childPath
        return ($Start + 1)
    }
    if ($Particle.LocalName -eq 'group') {
        $order = $Particle.GetAttribute('order')
        if ([string]::IsNullOrWhiteSpace($order)) { $order = 'seq' }
        if ($order -ne 'seq') {
            Add-ValidationError "Unsupported XDR group order '$order' at $Path; validation fails closed."
            return @()
        }
        $children = @(Get-XdrParticles -Declaration $Particle)
        $ends = @(Match-XdrSequence -Particles $children -Actual $Actual -Start $Start -Path $Path)
        return @($ends | Where-Object { $_ -gt $Start })
    }
    Add-ValidationError "Unsupported XDR particle '$($Particle.LocalName)' at $Path."
    return @()
}
function Match-XdrParticle {
    param([System.Xml.XmlNode]$Particle, [System.Xml.XmlElement[]]$Actual, [int]$Start, [string]$Path)
    $bounds = @(Get-OccurrenceBounds -Particle $Particle -Remaining ($Actual.Count - $Start))
    $minimum = [int]$bounds[0]
    $maximum = [int]$bounds[1]
    $accepted = [System.Collections.Generic.List[int]]::new()
    $states = [System.Collections.Generic.List[int]]::new()
    $states.Add($Start)
    if ($minimum -eq 0) { $accepted.Add($Start) }
    $count = 0
    while ($count -lt $maximum -and $states.Count -gt 0) {
        $next = [System.Collections.Generic.List[int]]::new()
        foreach ($state in $states) {
            foreach ($end in @(Match-XdrSingle -Particle $Particle -Actual $Actual -Start $state -Path $Path)) {
                if ($end -gt $state) { $next.Add([int]$end) }
            }
        }
        $uniqueNext = @(Get-UniqueStates -States $next.ToArray())
        if ($uniqueNext.Count -eq 0) { break }
        $count++
        $states = [System.Collections.Generic.List[int]]::new()
        foreach ($state in $uniqueNext) { $states.Add([int]$state) }
        if ($count -ge $minimum) { foreach ($state in $states) { $accepted.Add($state) } }
    }
    return (Get-UniqueStates -States $accepted.ToArray())
}
function Match-XdrSequence {
    param([System.Xml.XmlNode[]]$Particles, [System.Xml.XmlElement[]]$Actual, [int]$Start, [string]$Path)
    $states = [System.Collections.Generic.List[int]]::new()
    $states.Add($Start)
    foreach ($particle in $Particles) {
        $next = [System.Collections.Generic.List[int]]::new()
        foreach ($state in $states) {
            foreach ($end in @(Match-XdrParticle -Particle $particle -Actual $Actual -Start $state -Path $Path)) {
                $next.Add([int]$end)
            }
        }
        $unique = @(Get-UniqueStates -States $next.ToArray())
        $states = [System.Collections.Generic.List[int]]::new()
        foreach ($state in $unique) { $states.Add([int]$state) }
        if ($states.Count -eq 0) { break }
    }
    return ($states.ToArray())
}
function Test-XdrElement {
    param([System.Xml.XmlElement]$Element, [string]$TypeName, [string]$Path)
    if (-not $script:ElementTypes.ContainsKey($TypeName)) {
        Add-ValidationError "No XDR ElementType '$TypeName' is declared (at $Path)."
        return
    }
    foreach ($attribute in $Element.Attributes) {
        if ($attribute.NamespaceURI -eq 'http://www.w3.org/2000/xmlns/') { continue }
        Add-ValidationError "Unexpected XML attribute '$($attribute.Name)' at $Path."
    }
    $declaration = $script:ElementTypes[$TypeName]
    $content = $declaration.GetAttribute('content')
    $children = [System.Xml.XmlElement[]]@(Get-ElementChildren -Node $Element)
    if ($content -eq 'textOnly') {
        if ($children.Count -gt 0) { Add-ValidationError "Text-only XDR element '$TypeName' contains child elements at $Path." }
        $datatype = $declaration.GetAttribute('type', 'urn:schemas-microsoft-com:datatypes')
        $value = $Element.InnerText.Trim()
        if ($datatype -match '^(int|i1|i2|i4|i8|ui1|ui2|ui4|ui8)$' -and $value -notmatch '^[+-]?\d+$') {
            Add-ValidationError "Value '$value' is not an integer of XDR type '$datatype' at $Path."
        } elseif ($datatype -eq 'boolean' -and $value -notmatch '^(0|1|true|false)$') {
            Add-ValidationError "Value '$value' is not an XDR boolean at $Path."
        }
        return
    }
    if ($content -ne 'eltOnly') {
        if ($children.Count -gt 0) { Add-ValidationError "Unsupported XDR content model '$content' has child elements at $Path." }
        return
    }
    $particles = @(Get-XdrParticles -Declaration $declaration)
    $ends = @(Match-XdrSequence -Particles $particles -Actual $children -Start 0 -Path $Path)
    if ($ends -notcontains $children.Count) {
        $unmatched = ''
        foreach ($end in ($ends | Sort-Object -Descending)) {
            if ($end -lt $children.Count) {
                $unmatched = " First unmatched child: '$($children[$end].LocalName)' at position $($end + 1)."
                break
            }
        }
        Add-ValidationError "Child elements do not match the XDR sequence for '$TypeName' at $Path.$unmatched"
    }
}
function Test-Civ4XdrParserCompatibility {
    param([System.Xml.XmlDocument]$SchemaDocument)
    foreach ($type in $SchemaDocument.SelectNodes('//*[local-name()="ElementType"]')) {
        $typeName = $type.GetAttribute('name')
        foreach ($particle in $type.SelectNodes('.//*[@maxOccurs]')) {
            $maximum = $particle.GetAttribute('maxOccurs')
            if ($maximum -notin @('1', '*')) {
                Add-ValidationError "Civ4's MSXML 3 XDR parser accepts maxOccurs only as '1' or '*'; found '$maximum' in ElementType '$typeName'."
            }
        }
    }
}
function Test-StandardYieldArrayLengths {
    param([System.Xml.XmlDocument]$DataDocument, [System.Xml.XmlDocument]$SchemaDocument)
    $yieldArrayTypes = @{}
    foreach ($type in $SchemaDocument.SelectNodes('//*[local-name()="ElementType"]')) {
        $typeName = $type.GetAttribute('name')
        foreach ($particle in $type.SelectNodes('.//*[local-name()="element"][@maxOccurs="*"]')) {
            $valueType = $particle.GetAttribute('type')
            if ($valueType -in @('iYield', 'iYieldChange')) { $yieldArrayTypes[$typeName] = $valueType }
        }
    }
    foreach ($arrayType in $yieldArrayTypes.Keys) {
        $valueType = $yieldArrayTypes[$arrayType]
        foreach ($container in $DataDocument.SelectNodes("//*[local-name()='$arrayType']")) {
            $valueCount = @(Get-ElementChildren -Node $container | Where-Object { $_.LocalName -eq $valueType }).Count
            if ($valueCount -gt 3) {
                Add-ValidationError "Standard yield array '$arrayType' has $valueCount '$valueType' values; the receiving DLL supports NUM_YIELD_TYPES=3."
            }
        }
    }
}

foreach ($path in @($XmlPath, $SchemaPath)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "File not found: $path" }
}
$xmlBytes = [System.IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $XmlPath).Path)
if ($xmlBytes.Length -ge 3 -and $xmlBytes[0] -eq 0xEF -and $xmlBytes[1] -eq 0xBB -and $xmlBytes[2] -eq 0xBF) {
    throw "UTF-8 BOM is not accepted by this Civ4 XML pipeline: $XmlPath"
}
$readerSettings = [System.Xml.XmlReaderSettings]::new()
$readerSettings.DtdProcessing = [System.Xml.DtdProcessing]::Prohibit
$readerSettings.XmlResolver = $null
$xmlReader = [System.Xml.XmlReader]::Create((Resolve-Path -LiteralPath $XmlPath).Path, $readerSettings)
$document = [System.Xml.XmlDocument]::new()
$document.XmlResolver = $null
try { $document.Load($xmlReader) } finally { $xmlReader.Dispose() }
$schemaReader = [System.Xml.XmlReader]::Create((Resolve-Path -LiteralPath $SchemaPath).Path, $readerSettings)
$schemaDocument = [System.Xml.XmlDocument]::new()
$schemaDocument.XmlResolver = $null
try { $schemaDocument.Load($schemaReader) } finally { $schemaReader.Dispose() }
if ($schemaDocument.DocumentElement.LocalName -ne 'Schema' -or $schemaDocument.DocumentElement.NamespaceURI -ne 'urn:schemas-microsoft-com:xml-data') {
    throw "Not a Microsoft XDR schema: $SchemaPath"
}
foreach ($type in $schemaDocument.SelectNodes('//*[local-name()="ElementType"]')) {
    $name = $type.GetAttribute('name')
    if ([string]::IsNullOrWhiteSpace($name)) { throw "XDR schema has an unnamed ElementType: $SchemaPath" }
    if ($script:ElementTypes.ContainsKey($name)) { throw "Duplicate XDR ElementType '$name' in $SchemaPath" }
    $script:ElementTypes[$name] = $type
}
Test-Civ4XdrParserCompatibility -SchemaDocument $schemaDocument
$rootName = $document.DocumentElement.LocalName
if ($document.DocumentElement.NamespaceURI -ne "x-schema:$([System.IO.Path]::GetFileName($SchemaPath))") {
    throw "XML namespace '$($document.DocumentElement.NamespaceURI)' does not identify the supplied schema '$([System.IO.Path]::GetFileName($SchemaPath))'."
}
Test-XdrElement -Element $document.DocumentElement -TypeName $rootName -Path "/$rootName"
Test-StandardYieldArrayLengths -DataDocument $document -SchemaDocument $schemaDocument
if ($script:ValidationErrors.Count -gt 0) {
    if ($ContinueOnError) {
        Write-Output "XDR STRUCTURE FAIL: $XmlPath"
        foreach ($errorMessage in $script:ValidationErrors) { Write-Output " - $errorMessage" }
        return
    }
    [Console]::Error.WriteLine("XDR STRUCTURE FAIL: $XmlPath")
    foreach ($errorMessage in $script:ValidationErrors) { [Console]::Error.WriteLine(" - $errorMessage") }
    exit 1
}
Write-Output "XDR STRUCTURE PASS: $XmlPath against $SchemaPath"
