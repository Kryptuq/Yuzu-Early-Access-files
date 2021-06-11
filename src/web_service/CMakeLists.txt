add_library(web_service STATIC
    telemetry_json.cpp
    telemetry_json.h
    verify_login.cpp
    verify_login.h
    web_backend.cpp
    web_backend.h
    web_result.h
)

create_target_directory_groups(web_service)
target_link_libraries(web_service PRIVATE common nlohmann_json::nlohmann_json httplib)
