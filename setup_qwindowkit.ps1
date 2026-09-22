# Initialize QWindowKit submodule and build
Write-Host "Initializing git submodules..."
git submodule update --init --recursive

Write-Host "Creating QWindowKit build directory..."
New-Item -ItemType Directory -Path "third_party\qwindowkit\build" -Force

Write-Host "Generating QWindowKit Visual Studio project..."
& "C:\Program Files\CMake\bin\cmake.exe" -G "Visual Studio 17 2022" -A x64 `
    -DQWINDOWKIT_BUILD_EXAMPLES=OFF `
    -DQWINDOWKIT_BUILD_TESTS=OFF `
    third_party\qwindowkit -B third_party\qwindowkit\build

Write-Host "Building QWindowKit (Debug)..."
& "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" `
    third_party\qwindowkit\build\qwindowkit.sln `
    /p:Configuration=Debug `
    /p:Platform=x64 `
    /t:Build `
    /v:minimal

Write-Host "Building QWindowKit (Release)..."
& "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" `
    third_party\qwindowkit\build\qwindowkit.sln `
    /p:Configuration=Release `
    /p:Platform=x64 `
    /t:Build `
    /v:minimal

Write-Host "QWindowKit setup complete!"
