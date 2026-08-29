# regex モジュールは cplat で唯一 C++ を使用する。
# C++ 標準ライブラリのテンプレートは本モジュールで実体化され、既定のままでは
# libcplat.so の動的シンボル表へ多数のマングル済みシンボルとして現れる。
# cplat の公開 ABI は C 関数のみであるため、既定の可視性を hidden にして
# 実体化されたテンプレートを共有ライブラリの外へ出さない。
# 公開 API の宣言は regex.cc 側の #pragma GCC visibility push(default) で
# 既定可視性へ戻している。
#
# MSVC は __declspec(dllexport) を付けたシンボルのみをエクスポートするため、
# 同種の設定は不要。
ifdef PLATFORM_LINUX
    CXXFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden
endif
