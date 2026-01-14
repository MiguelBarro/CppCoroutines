# Script to generate the C++ projection of the WinRT components used in the examples
if (-not (gcm cl -ErrorAction SilentlyContinue))
{
    $vswhere = "${Env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    & $vswhere -find **/Microsoft.VisualStudio.DevShell.dll | Import-Module
    Enter-VsDevShell -SetDefaultWindowTitle -InstallPath (& $vswhere -property installationPath) `
                     -StartInPath $pwd -DevCmdArguments '/arch=x64 /host_arch=x64'
}

# Base winmd file
$metafile = ls -Path "${env:ProgramFiles(x86)}\Windows Kits" -R -Filter Windows.Foundation.FoundationContract.winmd |
             ? DirectoryName -match "${Env:WindowsSDKVersion}\"

# all UWP winmd files
$metas = ls -Path "${env:ProgramFiles(x86)}\Windows Kits" -R -Filter *.winmd | ? DirectoryName -match "${Env:WindowsSDKVersion}\"

# search for socket winmd
$pattern = "StreamSocket|WwanConnectionProfileDetails|Windows.Storage.Streams"
$depend_metas = $metas | ? {
        (ildasm /TEXT /NOIL /CLASSLIST $_ | sls -Pattern $pattern) -ne $null
    }

# Generate the projection
$projdir = "$PSScriptRoot\projection"
if (Test-Path $projdir) { rm -recurse -force $projdir }
mkdir $projdir

cppwinrt /verbose /optimize /base /output $projdir /reference $metafile /input ($depend_metas | % { "/input", $_ })

# Trim those files that are not necessary
$sources = gi "$PSScriptRoot\*.cpp"
pushd $Env:TEMP
$depends = $sources | % {
    (cl /nologo /std:c++20 /EHsc /I $projdir /sourceDependencies- $_ /link windowsapp.lib |
    select -Skip 1 | ConvertFrom-Json).Data.Includes
} | sort | Get-Unique | gi | ? Fullname -like $projdir*
popd

(Compare-Object -ReferenceObject $depends -DifferenceObject ($files = ls -Path $projdir -R -File) |
    ? SideIndicator -like "=>").InputObject | rm
