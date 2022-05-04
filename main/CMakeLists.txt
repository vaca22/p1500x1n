idf_component_register(SRCS "main_app.c"
        "http_server.c" "myble.c" "dns_server.c"
        INCLUDE_DIRS "."
        EMBED_FILES "index.html")
