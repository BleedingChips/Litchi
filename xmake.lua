add_rules("mode.debug", "mode.release")
set_languages("cxxlatest")

add_requires("zlib")
add_requires("minizip")
add_requires("asio")

if os.scriptdir() == os.projectdir() then 
    includes("../Potato/")
end

target("Litchi")
    set_kind("static")
    add_files("Litchi/*.ixx")
    add_files("Litchi/*.cpp")
    add_packages("asio")
    add_deps("Potato")
    add_packages("zlib")
    add_packages("minizip")
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