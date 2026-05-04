set windows-shell := ["powershell.exe", "-NoLogo", "-Command"]

# Build the app with either Release or Debug mode
[arg('type', pattern='(?i)Release|Debug')]
build type='Debug':
    cmake -DCMAKE_BUILD_TYPE={{type}} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -S . -B build
    cmake --build build --config {{type}}

# Build the app as a website with either Release or Debug mode
[arg('type', pattern='(?i)Release|Debug')]
build-web type='Debug':
    emcmake cmake -DCMAKE_BUILD_TYPE={{type}} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -S . -B build
    cmake --build build --config {{type}} -- -j


recurse_delete := if os_family() == "windows" { "Remove-Item -Recurse -Force" } else { "rm -rf " }

# Delete the build folder
clean:
    {{ recurse_delete }} ./build/

