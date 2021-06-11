add_library(sirit
    ../include/sirit/sirit.h
    sirit.cpp
    op.cpp
    op.h
    stream.cpp
    stream.h
    operand.cpp
    operand.h
    literal_number.cpp
    literal_number.h
    literal_string.cpp
    literal_string.h
    common_types.h
    instructions/type.cpp
    instructions/constant.cpp
    instructions/function.cpp
    instructions/flow.cpp
    instructions/debug.cpp
    instructions/memory.cpp
    instructions/annotation.cpp
    instructions/misc.cpp
    instructions/logical.cpp
    instructions/conversion.cpp
    instructions/bit.cpp
    instructions/arithmetic.cpp
    instructions/extension.cpp
    instructions/image.cpp
    instructions/group.cpp
    instructions/barrier.cpp
    instructions/atomic.cpp
)

target_compile_options(sirit PRIVATE ${SIRIT_CXX_FLAGS})

target_include_directories(sirit
                           PUBLIC ../include
                           PRIVATE . ${SPIRV-Headers_SOURCE_DIR}/include
                           INTERFACE ${SPIRV-Headers_SOURCE_DIR}/include)
