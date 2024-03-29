﻿
# spcmake_byFF4 HexMMLコマンド一覧

## コメントアウト

// 以降改行までコメントアウト

    F2 00 00 A0 // 音量

/\* から \*/ まで複数行コメントアウト

    /*
    B4 B6
    B8
    */
    C9

## 以下トラック記述前に定義するもの

## #songname "_kyokumei_"

曲名を設定します。

## #gametitle "_ge-mumei_"

ゲーム曲であれば設定します。

## #artist "_Yamada Taro_"

作曲者を設定します。

## #dumper "_pgate1_"

HexMMLの作成者を設定します。つまりあなたの名前。

## #comment "_nanikakaku_"

SPCプレイヤで表示されるコメントを設定します。

## #length _play_time fade_out_time_

曲の再生時間とフェードアウト時間(ms)を設定します。
デフォルトでは再生時間( _play_time_ )300秒、フェードアウト時間( _fade_out_time_ )10000msとなっています。
SPCプレイヤによって対応していたり対応していなかったり。

    #length 2:30 4000 // 再生時間2分30秒、フェードアウト時間4秒
    #length 150 10000 // 再生時間150秒、フェードアウト時間10秒

## #brr_offset _offset_

BRRデータのAPUメモリ内配置位置を指定するオプションです。
デフォルトは _offset_ = 0x3000です。

    #brr_offset 0x4B00 // BRRデータを0x4B00から配置します
    #brr_offset auto // BRRデータをシーケンスデータの直後に配置します

## #brr_echo_overcheck

このオプション設定を記述すると、BRRデータ領域がエコーバッファ領域と重なっていないかチェックし、重なっている場合はエラーとします。  
エコーバッファ領域はデフォルトで0xD000～0xF7FFとなっています。

## #echo_depth _depth_

エコーのかかり具合を指定します。  
_depth_ はエコーの深さです。1(浅い)～15(深い)の間で指定してください。デフォルトは5です。

## #surround

逆位相サラウンドを有効にするオプション設定です。  
MVOL_Rに0x40の代わりに0xC0を設定します。

## #swap<> #swap><

オクターブ変更コマンド < \> の機能を入れ替えます。

## #auto_assign_toneid

これを宣言すると、tone宣言で _tone_id_ を省略できます。
省略された _tone_id_ には64から順に内部idが割り当てられます。

## #tone (音色定義)

音色を設定します。
基本的に音色を使用するトラック宣言の前に定義します。

#tone _tone_id "brr_file_name" tuning attack_type sustain_rate release_rate_

    #tone 1 "brr/harp.brr" 00 00 0A 3B // 自分で用意したBRRファイルを使用する場合
    #tone 2 "FF4inst:1" 00 0C 20 // FF4内蔵波形を使用（パラメータ指定あり）
    #tone 3 "FF4inst:5" // FF4内蔵音源を使用（パラメータ指定なし）
    #tone 4 "FF4inst:s0" 00 0C 20 // FF4常駐音源を使用（パラメータ指定あり）
    #tone 5 "FF4inst:s3" // FF4常駐音源を使用（パラメータ指定なし）

_tone_id_ 数字（例：1, 3, 0A, 1C）や、文字列（例：base, TAM）を宣言します。
  
FF4内蔵波形はFF4inst:00～16(16進数)を指定できます。00は無音です。  
FF4常駐波形はFF4inst:s0～s6を指定できます。  
パラメータは16進数で記述してください。  
FF4instを指定する場合にパラメータを省略することができますが、アタックレート等は埋め込まれないためシーケンスで記述する必要があります。
  
自前の.brrを使用する場合は4つのパラメータが必須です。  
_tuning_ 音程補正を指定します。80～7F（-128～127）で指定してください。  
_attack_type_ コマンドは DC XX に変換されます。  
_sustain_rate_ コマンドは DD XX に変換されます。  
_release_rate_ コマンドは DE XX に変換されます。  

今のところtone宣言は32個までとさせてください。

## #track _track_num_

トラックを設定します。
スーファミ音源は8チャンネルなので、 _track_num_ は1～8で設定できます。
これ以降、次の#trackまでがそのトラックのシーケンスとなります。

## #macro _macro_key_ "_macro_value_"

マクロを定義します。  
これ以降の記述において、 _macro_key_ が _macro_value_ に置き換えられます。

    #macro baseA "1E < 96 > 00 1E"
    ...
    [ baseA ]4 // baseA が 1E < 96 > 00 1E に置き換えられる

## L

トラックループです。
これ以降のシーケンスがトラック終了からループします。  
シーケンスでは最後に制御コマンド F1 が追加されます。

## @_n_

#toneで定義した音色を選択します。

    @1 @3 @0A @1C @base @TAM

## [...]_n_

ループです。[ ] 内を _n_ 回繰り返します。多重ループも可能です。  
_n_ は2以上を指定してください。0と1は無限ループです。  
シーケンスでは [ が E0 に、] が F0 に置き換えられます。

    [ 26 8F ]2 // 26 8F 26 8F

    [ [ 26 8F ]2 44 08 ]2 // 26 8F 26 8F 44 08 26 8F 26 8F 44 08 

## | (パイプ)

ループブレイクポイントで、ループの最後にループを抜けます。  
シーケンスでは | が F4 に置き換えられます。  
多重ループの中でも使用できます。

    [ 26 | 8F ]3 08 // 26 8F 26 8F 26 08

## > <

オクターブ操作です。\> で +1オクターブ、< で -1オクターブします。  
シーケンスでは > は E1 に、< は E2 に置き換えられます。

## J _label_

トラック内任意場所へのジャンプで、順方向ジャンプ、逆方向ジャンプが可能です。  
異なるトラックであれば同じラベル名を使用できます。  
注意、本コマンドの使用によりticks計算が正確でなくなる可能性があります。

_label_ 任意文字列可。

    82 84
    J lp // ラベルlpへジャンプ
    66 66 66
    lp:  // ジャンプ先ラベル
    4A 4C
    J lp // ラベルlpへジャンプ
    20

    結果：82 84 4A 4C 4A 4C ...

## d

10進数を記述します。使用できるのはトラック内のみです。

    d93 // 5Dに変換される
    d-15 // F1に変換される

## @@_n_

効果音をシーケンスに埋め込みます。  
_n_ は効果音番号で 0～255 までです。

## !

この記号以降のコンパイルをストップします。

以上。
