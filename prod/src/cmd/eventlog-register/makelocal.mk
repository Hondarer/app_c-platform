# eventlog-register 専用のローカル設定
#
# Windows のメッセージ テーブル リソース (MESSAGETABLE) を eventlog-register.exe に
# 埋め込む。eventlog_messages.mc を mc.exe -> rc.exe で .res 化し、
# link.exe に直接渡す。
# 登録時に EventMessageFile / CategoryMessageFile へ自身の絶対パスを書くことで、
# Event Viewer の補完文を抑止し、本文とカテゴリ名のみを表示させる。
#
# Linux ではイベント ログを使用しないため、本ファイルの内容は無効となる。

ifdef PLATFORM_WINDOWS

# 生成物は OBJDIR (obj/<CRT>) 配下に置く。makefw の clean (rm -rf obj) で消える。
# OBJDIR は makesrc 側で定義されるが、本ファイルの読み込み時点では未定義のため、
# 同じ値 (obj/$(MSVC_CRT_SUBDIR)) をここで組み立てる。
MC_OBJDIR := obj/$(MSVC_CRT_SUBDIR)
MC_SRC    := eventlog_messages.mc
MC_RC     := $(MC_OBJDIR)/eventlog_messages.rc
MC_RES    := $(MC_OBJDIR)/eventlog_messages.res

# メッセージ リソースの .res を生成する。
# mc.exe   : .mc -> ヘッダー / .rc / MSG00001.bin (Unicode メッセージ: -U)
# rc.exe   : .rc -> .res (MSG00001.bin を /i で解決)
# .res は link.exe へ直接渡す。cvtres.exe で .obj 化すると、/MANIFEST:EMBED
# が生成する .rsrc と属性が一致せず LNK4078 の原因になる。
$(MC_RES): $(MC_SRC) makelocal.mk
	@mkdir -p $(MC_OBJDIR)
	@rm -f $(MC_OBJDIR)/eventlog_messages*.obj
	@echo "mc.exe -U $(MC_SRC)"
	@MSYS_NO_PATHCONV=1 mc.exe -U -h $(MC_OBJDIR) -r $(MC_OBJDIR) $(MC_SRC)
	@echo "rc.exe $(MC_RC)"
	@MSYS_NO_PATHCONV=1 rc.exe /nologo /i $(MC_OBJDIR) /fo $(MC_RES) $(MC_RC)

# リンクより前に .res が生成されていることを保証し、link.exe に直接渡す。
LINK_INPUTS += $(MC_RES)

endif # PLATFORM_WINDOWS
