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

終了後、カーソルが画面上端へ移動しないこと。次のシェルプロンプトは、pinned prompt があった位置に表示されること。

## 実装時の注意

- 通常出力前は `main_bottom_row` へ移動し、必要に応じて VT scroll region を `1..main_bottom_row` に制限する。
- 通常出力後は `\033[r` で scroll region を解除してから status / separator / prompt を再描画する。
- prompt 表示中は prompt 直上 separator を常に描画する。
- top status 有効時は `top status -> separator -> prompt` を連続表示し、間に空行を入れない。
- control block の上に見える空行は 1 行だけにする。
- `dispose()` では通常の prompt 非表示ではなく、終了用 cleanup で status / separator / prompt 領域も消去する。
