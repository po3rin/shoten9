= たのしいUML

== はじめに
TODO:

=== この章のゴール
TODO:

=== 検証環境
この章では、次の環境で検証しています。

 * macOS 10.15.6
 * Go 1.15.1

== UMLとは
UML（Unified Modeling Language）@<fn>{UML}は、設計をモデル化するときの記法を統一した言語です。システムの構造や振る舞いを図で表すことができ、次のようなバリエーションがあります。（一部のみ抜粋）
//footnote[UML][https://www.uml.org]

 : 構造図
 * クラス図
 * コンポーネント図
 * オブジェクト図
 * パッケージ図

 : 振る舞い図
 * ユースケース図
 * アクティビティ図
 * 状態遷移図
 * シーケンス図

//image[001][シーケンス図の例][width=0.5\maxwidth]

UMLを書く方法はさまざまです。Lucidchart@<fn>{Lucidchart}、Cacoo@<fn>{Cacoo}、draw.io@<fn>{draw.io}など、オンラインのツールは豊富です。
さらに、テキストでUMLを書くこともできます。テキストで書けるということは変更管理のしやすさを鑑みても大変魅力的です。
そのためのツールのひとつであるPlantUML@<fn>{PlantUML}について、この章では紹介をしていきます。
//footnote[Lucidchart][https://www.lucidchart.com/pages/ja/examples/uml-tool]
//footnote[Cacoo][https://cacoo.com/ja/]
//footnote[draw.io][https://app.diagrams.net]
//footnote[PlantUML][https://plantuml.com]

== PlantUML
PlantUMLは、テキストで書いたUMLを画像に出力することができるツールです。シーケンス図においてはアスキーアートで出力することができます。
さらにUML以外にも、ワイヤーフレームやER図、ガントチャートなどにも対応しており、常に機能がアップデートされ続けています。

図2.1は次のコードから生成されています。

//emlist[図2.1のコード]{
@startuml

participant "自転車" as Bicycle #lightpink
participant "クロクマ" as BlackBear #lightskyblue

Bicycle -> BlackBear :ぶつかる
BlackBear -> BlackBear: 転ぶ

Bicycle -> BlackBear: 謝る
activate BlackBear
BlackBear -->Bicycle: 許すまじ...
deactivate BlackBear

Bicycle -> Bicycle: 逃げる

@enduml
//}

=== PlantUMLのインストール
PlantUMLを実行するには、Javaの実行環境が不可欠です。まずはJavaの環境を用意しておきましょう。

次に必要なものはグラフデータを描画するためのGraphviz@<fn>{GraphvizDownload}です。インストールしておきましょう。
//footnote[GraphvizDownload][http://www.graphviz.org/download/]

最後に、PlantUMLの実行ファイルはダウンロードページ@<fn>{PlantUMLDownload}から入手できます。
//footnote[PlantUMLDownload][https://plantuml.com/ja/download]

macOSの場合は、Homebrewでもインストールできます。

//cmd{
$ brew install graphviz
$ brew install plantuml
//}

=== PlantUMLでUMLを生成する
UMLを書いたテキストファイルを指定することで、図（png）が生成されます。ファイルが格納されたディレクトリを指定することもできます。

//cmd{
$ java -jar plantuml.jar {UMLを書いたファイル}
//}

もしくは、Homebrewでインストールした場合は、次のコマンドです。

//cmd{
$ plantuml {UMLを書いたファイル}
//}

なお、PlantUMLの豊富なオプションは@<code>{-h}で確認することができます。

//cmd{
plantuml -h
Usage: java -jar plantuml.jar [options] -gui
	(to execute the GUI)
    or java -jar plantuml.jar [options] [file/dir] [file/dir] [file/dir]
	(to process files or directories)

You can use the following wildcards in files/dirs:
	*	means any characters but '/'
	?	one and only one character but '/'
	**	means any characters (used to recurse through directories)

where options include:
    -gui		To run the graphical user interface
    -tpng		To generate images using PNG format (default)
    -tsvg		To generate images using SVG format
    -teps		To generate images using EPS format
    -tpdf		To generate images using PDF format
    .
    .
    .
//}

== PlantUMLをGoで描く
UMLをテキストで書けるようになりました。
この章では、jfeliu007/goplantuml@<fn>{goplantuml}というGoのパッケージを利用することで、Goのコードからクラス図を生成する方法を紹介します。
//footnote[goplantuml][https://github.com/jfeliu007/goplantuml/]

=== goplantumlのインストール
TODO: 文章

=== Goでクラスを書く
TODO: 文章

//emlist[bear/bear.go][go]{
package bear

import "fmt"

type BearInterface interface {
	Sleep()
	Eat()
}

type Bear struct {
	Name  string
	Color string
}

func (b *Bear) Sleep() {
	fmt.Println("眠る")
}

func (b *Bear) Eat() {
	fmt.Println("食べる")
}
//}

//emlist[bear/blackbear.go][go]{
package bear

import "fmt"

type BlackBear struct {
	Bear
}

func (b *BlackBear) climbing() {
	fmt.Println("木登りをする")
}
//}

//emlist[bear/polerbear.go][go]{
package bear

import "fmt"

type PolerBear struct {
	Bear
}

func (p *PolerBear) swim() {
	fmt.Println("海で泳ぐ")
}
//}

実行コマンド
//cmd{
$ goplantuml ./bear > bear.puml
//}

実行後に生成されたUML:
//emlist{
@startuml
namespace bear {
    class Bear << (S,Aquamarine) >> {
        + Name string
        + Color string

        + Sleep() 
        + Eat() 

    }
    interface BearInterface  {
        + Sleep() 
        + Eat() 

    }
    class BlackBear << (S,Aquamarine) >> {
        - climbing() 

    }
    class PolerBear << (S,Aquamarine) >> {
        - swim() 

    }
}
"bear.Bear" *-- "bear.BlackBear"
"bear.Bear" *-- "bear.PolerBear"

"bear.BearInterface" <|-- "bear.Bear"

@enduml
//}

生成後:
//image[002][Bearクラス][width=0.5\maxwidth]

#@# その他:
#@# https://github.com/steinfletcher/apitest-plantuml
#@# https://github.com/yogendra/plantuml-go
#@# https://github.com/kazukousen/gouml

== さいごに
TODO: