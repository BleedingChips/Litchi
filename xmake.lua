add_rules("mode.debug", "mode.release")
set_languages("cxxlatest")

if os.scriptdir() == os.projectdir() then
    set_project("Litchi")
    includes("../Potato/")
end

target("Litchi")
    set_kind("static")
    add_files("Litchi/*.cpp")
    add_files("Litchi/*.ixx")
    add_deps("Potato")
target_end()

if os.scriptdir() == os.projectdir() then
    set_project("Litchi")

    for _, file in ipairs(os.files("Test/*.cpp")) do

        local name = path.basename(file)

        target(name)
            set_kind("binary")
            add_files(file)
            add_deps("Litchi")
        target_end()

    end
end






