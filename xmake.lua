add_rules("mode.debug", "mode.release")
set_languages("cxxlatest")

if os.scriptdir() == os.projectdir() then
    set_project("Litchi")
    includes("../Potato/")
end

add_requires("zlib")
add_requires("asio")

target("LitchiAsioWrapper")
    set_kind("static")
    add_files("Litchi/AsioWrapper/*.cpp")
    add_headerfiles("Litchi/AsioWrapper/*.h")
    add_packages("asio")
    add_defines("ASIO_NO_DEPRECATED")
target_end()

target("Litchi")
    set_kind("static")
    add_files("Litchi/*.cpp")
    add_files("Litchi/*.ixx")
    add_deps("LitchiAsioWrapper")
    add_deps("Potato")
    add_packages("zlib")
    add_defines("ZLIB_CONST")
target_end()

if os.scriptdir() == os.projectdir() then
    set_project("Litchi")

    for _, file in ipairs(os.files("Test/*.cpp")) do

        local name = "ZTest_" .. path.basename(file)

        target(name)
            set_kind("binary")
            add_files(file)
            add_deps("Litchi")
        target_end()

    end
end






