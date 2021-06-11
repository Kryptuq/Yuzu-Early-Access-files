Sirit
=====
A runtime SPIR-V assembler. It aims to ease dynamic SPIR-V code generation
without calling external applications (like Khronos' `spirv-as`)  

Its design aims to move code that does not belong in the application to the
library, without limiting its functionality.

What Sirit does for you:
* Sort declaration opcodes
* Handle types and constant duplicates
* Emit SPIR-V opcodes  

What Sirit won't do for you:
* Avoid ID duplicates (e.g. emitting the same label twice)
* Dump code to disk
* Handle control flow
* Compile from a higher level language
  
  
It's in early stages of development, many instructions are missing since
they are written manually instead of being generated from a file.

Example
-------

```cpp
class MyModule : public Sirit::Module {
public:
    MyModule() {}
    ~MyModule() = default;

    void Generate() {
        AddCapability(spv::Capability::Shader);
        SetMemoryModel(spv::AddressingModel::Logical, spv::MemoryModel::GLSL450);
        
        auto main_type{TypeFunction(TypeVoid())};
        auto main_func{OpFunction(TypeVoid(), spv::FunctionControlMask::MaskNone, main_type)};
        AddLabel(OpLabel());
        OpReturn();
        OpFunctionEnd();

        AddEntryPoint(spv::ExecutionModel::Vertex, main_func, "main");
    }
};

// Then...

MyModule module;
module.Generate();

std::vector<std::uint32_t> code{module.Assemble()};
```
