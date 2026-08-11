# byteorder.h は static inline のみで構成されるため、テスト対象のソース ファイルはない。
# テスト実行体はヘッダーをインクルードして展開された実体を検証する。

# ライブラリの指定
LIBS += mock_libc
