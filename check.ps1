Rename-Item C:\Users\Sven\AppData\Local\yuzu\yuzu-windows-msvc-early-access C:\Users\Sven\AppData\Local\yuzu\yuzu-early-access

Compress-Archive -LiteralPath "C:\Users\Sven\AppData\Local\yuzu\yuzu-early-access" -DestinationPath "C:\Users\Sven\Desktop\yuzu-early-access-EA{{ github.event.head_commit.message }}.zip"

Rename-Item C:\Users\Sven\AppData\Local\yuzu\yuzu-early-access C:\Users\Sven\AppData\Local\yuzu\yuzu-windows-msvc-early-access
