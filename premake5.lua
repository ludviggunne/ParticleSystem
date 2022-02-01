workspace "ParticleSystem"
    architecture "x64"
    configurations {
        "Debug",
        "Release"
    }

    files {
        "premake5.lua",
        ".gitignore"
    }

    project "ParticleSystem"
        location "ParticleSystem"
        kind "ConsoleApp"
        language "C++"
        cppdialect "C++17"

        targetdir ("bin/%{cfg.buildcfg}/")
        objdir ("obj/%{cfg.buildcfg}/")

        files {
            "%{prj.name}/src/**.cpp",
            "C:/vclib/glad/src/glad.c",
            "%{prj.name}/resources/**.glsl"
        }

        includedirs {
            "C:/vclib/glad/include/",
            "C:/vclib/GLFW/include/",
            "C:/vclib/glm/include/"
        }

        libdirs {
            "C:/vclib/GLFW/%{cfg.buildcfg}/lib"
        }

        links {
            "opengl32.lib",
            "glfw3.lib"
        }

        filter "configurations:Debug"
            defines {
                "_DEBUG"
            }

            buildoptions "/MDd"

        filter "configurations:Release"
            buildoptions "/MD"