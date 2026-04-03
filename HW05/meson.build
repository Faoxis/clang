project(
  'hw05',
  'c',
  version: '0.1.0',
  meson_version: '>= 0.59.0',
  default_options: [
    'c_std=c11',
    'warning_level=3',
    'werror=false',
  ],
)

# Эквивалент -Wall -Wextra -Wpedantic задаётся через warning_level=3 для GCC/Clang.

gtk4 = dependency('gtk4')

executable('tree-app', 'src/main.c', dependencies: gtk4)

executable('gtk4_example', 'gtk4_example.c', dependencies: gtk4)
executable('file_example', 'src/directory/directory_parser.c', dependencies: gtk4)
executable('app_example', 'src/desktop/desktop_app.c', dependencies: gtk4)
