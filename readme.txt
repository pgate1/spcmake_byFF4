
       spcmake_byFF4.exe
                                 　　　　      by pgate1

FF4サウンドドライバやFF4波形を使ってSPCを生成します。
MMLはFF4サウンドドライバ向けに特化したものです。

※警告
生成されるSPCにはFF4のサウンドドライバや波形データが含まれています。
SPCデータを容易に公開されることはご遠慮ください。


▼ 使い方

ご自身でFF4カートリッジからFinalFantasy4.romをダンプする。
FinalFantasy4.romをspcmake_byFF4.exeと同じ場所に置く。
必要であれば.brrを用意する。
sample.txt のようなMMLを書く。
spcmake_byFF4.batをダブルクリック。
エラーが無ければsample.spcが生成される。

・コツ

制御コードは16進大文字で 0x 等はつけない。
制御コードはスペースを空ける（実は開けなくてもよい）。
#toneは@の上であればどこで定義しても良い。


▼ 履歴

2020/01/12
#brr_offset auto で常駐波形BRRを波形BRRの直後に置くようにした。
#tone の tuning 指定で音程補正値を埋め込むようにした。
#echo_depth でEDLを指定できるようにした。
@@n で効果音を鳴らせるように。

2020/01/11
初版。


------------------------------------------------------------------------
@brr890様にはサウンドドライバやMML等について多くのアドバイスを頂きました。
心より感謝申し上げます。

SPCプレイヤには黒羽製作所様の黒猫SPCを使用させていただきました。
各レジスタ表示が分かりやすくデバグが捗りました。

SPCのデータ表示や解析にはVGMTransを使用させていただきました。
シーケンスや波形データ位置などの確認がし易かったです。

また次のサイトも参考にさせていただきました。先人の情報共有に感謝します。
・SuperC
　https://github.com/boldowa/SuperC-SPCdrv
・FinalFantasyIV 制御コード
　http://gnilda.rosx.net/SPC/F4/command.html
・FFIVサウンド関係 ffbin@Wiki
　https://w.atwiki.jp/ffbin/pages/36.html


------------------------------------------------------------------------
pgate1
Web https://pgate1.at-ninja.jp
Twitter https://twitter.com/pgate1
