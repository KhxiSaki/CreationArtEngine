
project "CEEditor"
    kind "WindowedApp"
    language "C#"
    framework "4.8"
    uuid "E05DAF8B-CEB2-4DA6-92D5-BCEEC3747E06"

    files {
        "**.cs",
        "**.xaml",
        "**.csproj",
        "**.json"
    }
    
    -- Exclude generated files from build
    filter { "files:obj/**.cs", "files:obj/**.i.cs" }
        excludefrombuild "On"
        
    filter { "files:bin/**" }
        excludefrombuild "On"
        
    filter {}

    -- Enable WPF support for .NET Framework
    clr "on"
    flags { "WPF", "MultiProcessorCompile" }
    
    -- Required references for .NET Framework 4.8 WPF
    links {
        "System",
        "System.Core",
        "System.Data",
        "System.Xaml",
        "PresentationCore",
        "PresentationFramework",
        "WindowsBase"
    }
    
    filter "configurations:Debug"
        symbols "On"
        optimize "Off"
        defines { "DEBUG", "TRACE" }
        
    filter "configurations:Release"
        symbols "Off"
        optimize "On"
        defines { "RELEASE", "TRACE" }
        
    filter "configurations:Dist"
        symbols "On"
        optimize "Off"
        defines { "NDEBUG" }