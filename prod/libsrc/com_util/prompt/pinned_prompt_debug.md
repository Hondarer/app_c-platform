# pinned_prompt debug notes

## ビルド確認

`pinned_prompt.c` を変更した場合は、まず `prompt` ディレクトリで局所ビルドする。  
局所ビルドでもライブラリは生成される。

```bash
cd /home/user/c-modernization-kit/app/com_util/prod/libsrc/com_util/prompt
make
```

`make clean` は通常不要。ファイル名や生成物の構成を変えた場合だけ検討する。

CLI 単体の再リンクは以下で行う。

```bash
cd /home/user/c-modernization-kit/app/com_util/prod/src/cmd/pinned-prompt
make
```

`pinned_prompt.c` は `-Wpadded` 付きでコンパイルされる。構造体のメンバー配置を変更した場合は、`pinned_prompt.c` のコンパイル行で警告が出ていないか確認する。

差分の空白確認は以下で行う。

```bash
git -C /home/user/c-modernization-kit/app/com_util diff --check --
```

## PTY での手動確認

`pinned-prompt` は TTY でないと固定プロンプト描画に入らない。パイプ入力だけでは表示崩れの評価にならないため、実端末または Codex の PTY 実行で確認する。

起動コマンド:

```bash
cd /home/user/c-modernization-kit
app/com_util/prod/bin/pinned-prompt
```

Codex では `exec_command` の `tty: true` で起動し、返された `session_id` に `write_stdin` で入力を送る。

基本確認:

```text
status show top
status set-top-left 1234
echo test
```

期待する配置:

```text
echo: echo test
test

1234
-------------------------------------------------------------------------------
pinned-prompt>
```

`test` と top status の間に見える空行は 1 行だけにする。

bottom status 確認:

```text
status hide all
status show bottom
status set-bottom-left btm
echo bottom
```

期待する下部配置:

```text
-------------------------------------------------------------------------------
pinned-prompt>
-------------------------------------------------------------------------------
btm
```

top + bottom 確認:

```text
status show all
status set-top-left 1234
status set-bottom-left btm
echo both
```

期待する下部配置:

```text
1234
-------------------------------------------------------------------------------
pinned-prompt>
-------------------------------------------------------------------------------
btm
```

ANSI SGR 着色確認:

```text
echo \e[31mred\e[0m
status show top
status set-top-left \e[31mERROR\e[0m
status set-top-right RIGHT
status show bottom
status set-bottom-left \e[1;32mOK\e[0m
status set-top-left \e[38;2;255;0;0mTRUE\e[0m
```

期待する表示:

- `red`、`ERROR`、`TRUE` が赤、`OK` が明るい緑で表示されること。
- `RIGHT` の位置が、左側 status の SGR バイト数ではなく可視文字幅で決まること。
- `\e[31BROKEN` のように SGR ではないシーケンスは、幅 0 の着色指定として扱われないこと。

着色のコツ:

- CLI から入力する場合は、実 ESC 文字ではなく `\e` を使う。`pinned-prompt` が `\e` / `\E` を ESC に変換して API に渡す。
- 色を付けた区間の末尾には `\e[0m` を付ける。付け忘れると、それ以降の status、separator、prompt まで同じ属性で表示される場合がある。
- 左右 status を同時に使うときは、色指定を含む側だけでなく反対側の位置も確認する。SGR は表示幅 0 として扱われるため、右寄せ位置は見えている文字数で決まる。
- `\e[31m`、`\e[1;32m`、`\e[38;2;255;0;0m`、`\e[48;5;196m` のような `ESC [ ... m` 形式だけを着色用 SGR として扱う。カーソル移動や画面消去などの制御シーケンスは status 文字列に混ぜない。
- `echo` は通常出力 API の確認、`status set-*` は status の描画とレイアウト計算の確認に使う。

入力クリア確認:

```text
partial input
```

Enter を押さずに Esc キーを押し、`pinned-prompt>` の入力内容が空になることを確認する。

その後、別のコマンドを入力して Enter を押し、Esc 前の文字列が実行されないことを確認する。  
Esc は入力中の編集行だけを消去し、履歴そのものは削除しない。確認する場合は、先に `echo hist` を実行して履歴に追加し、次の入力中に Esc を押した後、上キーで `echo hist` が再表示されることを確認する。

呼び出し元別履歴リング確認:

```text
read primary
p1
read secondary
s1
read primary
```

3 回目の `read primary` で `primary>` が表示されたら、上キーを押して `p1` だけが再表示されることを確認する。Enter で確定してから、次を実行する。

```text
read secondary
```

`secondary>` が表示されたら、上キーを押して `s1` だけが再表示されることを確認する。`read primary` と `read secondary` は別の呼び出し元から `com_util_pinned_prompt_readline()` を呼ぶため、同じ `screen` でも履歴リングが分かれる。

`com_util_pinned_prompt_readline_fmt()` 経由も確認する。

```text
read formatted
f1
read formatted
```

2 回目の `read formatted` で `formatted>` が表示されたら、上キーを押して `f1` が再表示されることを確認する。

## worker 出力の確認

入力途中に通常出力が流れても、入力中の内容と status / separator / prompt が維持されることを確認する。

```text
status show all
start stdout
partial
```

しばらく待ち、`[stdout tick N]` が出ても `pinned-prompt> partial` が再描画されることを確認する。終了前に worker を止める。

```text
stop all
quit
```

## 終了時の確認

`quit` または `exit` で終了したときは、status / separator / prompt の予約領域を消去し、pinned prompt があった行の先頭にカーソルを戻す。

確認手順:

```text
status show all
status set-top-left 1234
status set-bottom-left btm
quit
```

終了後、カーソルが画面上端へ移動しないこと。次のシェル プロンプトは、pinned prompt があった位置に表示されること。

## 実装時の注意

- 通常出力前は `main_bottom_row` へ移動し、必要に応じて VT scroll region を `1..main_bottom_row` に制限する。
- 通常出力後は `\033[r` で scroll region を解除してから status / separator / prompt を再描画する。
- prompt 表示中は prompt 直上 separator を常に描画する。
- top status 有効時は `top status -> separator -> prompt` を連続表示し、間に空行を入れない。
- control block の上に見える空行は 1 行だけにする。
- `dispose()` では通常の prompt 非表示ではなく、終了用 cleanup で status / separator / prompt 領域も消去する。
