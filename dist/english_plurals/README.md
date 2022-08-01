# English Plurals

Qt has "Translation Rules for Plurals", small example

    // Take a source line like
    tr("Building: %n shader(s)", "", i)

    // i = 1:
    Building: 1 shader
    // i = 2:
    Building: 2 shaders

For yuzu the source language used is English, for all other languages handling of plurals is handled by Qt and the translation collaboration site. Handling plurals in the source language (English) requires special consideration.

With CMake flag GENERATE_QT_TRANSLATION a generated_en.ts file is created from the source. It ignored by git (`.gitignore` in the project root). It is placed in this directory so that the relative refrences with the source code is correct.

Having the plurals look nice isn't critical, and automation to use translation collaboration sites may require specifing the project language as "Pirate English", so this has been done manually.

The en.ts in this directory is taken from a build, edited in Qt Linguist and then committed. As the code is in XML, using the tool is not strictly required.
