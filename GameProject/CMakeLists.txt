project(thegame)

add_executable(thegame
    main.cpp
    Game.cpp
    World.cpp
)

target_include_directories(thegame PRIVATE
    ./
)
