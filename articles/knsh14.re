= パーサーを作って学ぶiCalendar

本章ではiCalendar形式をパースし、Goのオブジェクトに変換するライブラリの作成をします。
その過程でiCalendar形式の仕様に対しての理解を深めていきます。

== 概要
iCalendar形式はRFC 5545@<fn>{rfc_5545_link}で詳細な仕様が示されています。
RFC 5545@<fn>{rfc_5545_link}では次の仕様が定められています。

 1. iCalendar形式の文字コードに関する仕様
 2. iCalendarファイル内のデータの表現に関する仕様
 3. iCalendarで利用されるデータの型に関する仕様
 4. 各種コンポーネントに関する仕様

//footnote[rfc_5545_link][@<href>{https://tools.ietf.org/html/rfc5545}]

== iCalendarファイルの形式
iCalendar形式のファイルの文字コードはUTF-8でなければいけません。@<fn>{ical_character_set}
iCalendar形式のファイルはコンテツラインという単位で情報が記述されます。
iCalendarはコンポーネントと呼ばれるオブジェクトの塊で構成されています。

//footnote[ical_character_set][@<href>{https://tools.ietf.org/html/rfc5545#section-3.1.4}]

=== コンテンツライン
コンテンツラインはiCalendarの情報を記述する単位です。

@<list>{content_line_style}はコンテンツラインの形式です。

//list[content_line_style][コンテンツラインの形式]{
NAME;PARAM:VALUE
//}

@<tt>{NAME}はコンテンツラインの種類を示します。
コンポーネントの始まりと終わりや、それぞれのコンポーネントのプロパティの種類などがあります。

@<tt>{VALUE}は@<tt>{NAME}に対応する値です。
@<tt>{VALUE}は@<tt>{NAME}の項目によっては複数の@<tt>{VALUE}を設定できます。
複数の@<tt>{VALUE}はカンマで区切る必要があります。

コンテンツラインには少なくとも@<tt>{NAME}と@<tt>{VALUE}の2つはなければいけません。

@<tt>{PARAM}は必須ではない項目です。
@<tt>{NAME}に対応する@<tt>{VALUE}に更に情報を足すために使われます。
例えば、イベントの開始時刻のタイムゾーンや、複数のデータ型が使えるプロパティでデータの型名を明示したりする場合に用います。

コンテンツラインは基本的にはファイル内の各行が1つのコンテンツラインになります。
ただし1行は75オクテット以内でなければいけません。@<fn>{content_line_overview}
しかし、イベントの説明などの場合にこの制限を超える場合があります。
その場合にはコンテンツラインを複数行に分割する必要があります。
複数行に分割する際に2行目以降は行の先頭にスペース、もしくはタブ文字を挿入します。
余談ですが、世の中に出回っているiCalendarファイルにはこの規約を守っていないものも多くあります。

//footnote[content_line_overview][@<href>{https://tools.ietf.org/html/rfc5545#section-3.1}]

=== コンポーネント
iCalendar形式をパースする目的はこのコンポーネントのオブジェクトの生成です。
コンポーネントには次の種類が定義されています。

 * Calendarコンポーネント
 * Eventコンポーネント
 * Timezoneコンポーネント
 * TODOコンポーネント
 * Journalコンポーネント
 * FreeBusyコンポーネント
 * Standardコンポーネント
 * Daylightコンポーネント
 * Alarmコンポーネント

これらのコンポーネントが木構造になっています。
根のコンポーネントとして使えるのはCalendarコンポーネントのみです。
Eventコンポーネント、TODOコンポーネントにはAlarmコンポーネントが含まれる場合があります。
TimezoneコンポーネントにはサブコンポーネントとしてStandardコンポーネント、Daylightコンポーネントが含まれます。

それぞれのコンポーネントはプロパティを持っています。
CalendarコンポーネントはiCalのバージョンを示すプロパティを持っています。
Eventコンポーネントはイベントのタイトルや、終了時刻を持っています。
プロパティには必須の項目、必須ではないが、コンポーネント中に1つしかセットできない項目、0個以上複数設定できる項目があります。

== iCalendar形式のファイルをパースする流れ

iCalendar形式のファイルをパースしてGoのオブジェクトに変換するためにはいくつかのステップを経由して変換します。

 1. 文字列をコンテツラインに変換する
 2. コンテンツラインからコンポーネントのオブジェクトに変換する

=== 文字列をコンテンツラインに変換する
前半のステップとして、iCalendar形式のファイルの中身をコンテンツラインに変換します。
この章での入力を@<list>{input_raw_data}に、処理して得られるコンテンツラインを@<list>{expected_output_content_line}に示します。

//list[input_raw_data][想定される入力]{
BEGIN:VEVENT
DTSTART;VALUE=DATE:20211011
DTEND;VALUE=DATE:20211012
DTSTAMP:20200716T144532Z
UID:20211011_60o30d9l6ko32e1g60o30dr56k@google.com
SUMMARY:体育の日
END:VEVENT
//}

//list[expected_output_content_line][期待する出力結果であるコンテンツラインのスライス]{
[]*ContentLine{
  {
    Name: "BEGIN",
    Parameter: nil,
    Values: []string{"VEVENT"},
  },
  {
    Name: "DTSTART",
    Parameter: []Parameter{
      {
        Name: "VALUE",
        Values: []string{"DATE"}
      },
    },
    Values: []string{"20211011"},
  },
  {
    Name: "DTEND",
    Parameter: []Parameter{
      {
        Name: "VALUE",
        Values: []string{"DATE"}
      },
    },
    Values: []string{"20211012"},
  },
  {
    Name: "DTSTAMP",
    Parameter: nil,
    Values: []string{"20200716T144532Z"},
  },
  {
    Name: "UID",
    Parameter: nil,
    Values: []string{"20211011_60o30d9l6ko32e1g60o30dr56k@google.com"},
  },
  {
    Name: "SUMMARY",
    Parameter: nil,
    Values: []string{"体育の日"},
  },
  {
    Name: "END",
    Parameter: nil,
    Values: []string{"VEVENT"},
  },
}
//}

@<list>{input_raw_data}から@<list>{expected_output_content_line}への変換を実現するには、次のステップで実現します。

 1. 複数行を一つの文字列オブジェクトとして分割する
 2. 文字列をトークンの列に変換する
 3. トークンの列からコンテンツラインに変換する

==== 複数行を一つの文字列オブジェクトとして分割する
コンテンツラインは複数行で1つのコンテンツラインを形成する場合があります。
そのため、まず全ての行をチェックし、先頭の文字を確認します。
もし、スペースもしくはタブ文字なら複数行のコンテンツラインの一部と見なします。
最初の1文字を削り前の行と結合して、一つのコンテツラインになる文字列いきます。

@<list>{build_raw_lines_to_string_slice}に実際のコードを示します。
@<code>{bufio.Scanner}型を用いて1行ずつ読み込みながら、各行に対して先頭がスペースもしくはタブ文字かチェックを行います。
全てのチェックが終わると、コンテンツラインになる文字列のスライスに変換されます。
このスライスの各要素に対してコンテンツラインへの変換を行います。

//list[build_raw_lines_to_string_slice][文字列の集合から行ごとの文字列のスライスへの変換]{
var res []string
for scanner.Scan() {
	l := scanner.Text()
	switch {
	case strings.HasPrefix(l, " "):
		res[len(res)-1] += "\n" + strings.TrimPrefix(l, " ")
	case strings.HasPrefix(l, "\t"):
		res[len(res)-1] += "\n" + strings.TrimPrefix(l, "\t")
		continue
	default:
		res = append(res, l)
	}
}
//}

==== 文字列をトークンの列に変換する
@<tt>{DTSTART:19980118T073000Z}という文字列を@<list>{token_slice}に示すトークンのスライス形式に変換します。
トークンは単なる文字列を意味のある単位にまとめたものです。
例えば@<list>{token_slice}の1行目は@<tt>{IDENT}という識別子を表すトークンです。
その@<tt>{IDENT}という種類のトークンの値として@<tt>{DTSTART}を持っています。
トークンのスライスに変換すると、コンテンツラインに変換する際に処理が簡単になります。
#@# @<tt>{COLON}のトークンは、コンテンツラインの@<tt>{NAME}と@<tt>{VALUE}を分割する役目を持っています。
文字列からトークンの列へ変換する処理を字句解析と呼びます。

//list[token_slice][生成したいトークンのスライス]{
[]token.Token{
	{Type: token.IDENT, Value: "DTSTART"},
	{Type: token.COLON, Value: ":"},
	{Type: token.IDENT, Value: "19980118T073000Z"},
	{Type: token.EOF, Value: ""},
},
//}

#@# トークンの方はこれ
#@# 正規表現で変換するとParam のvalueの中でダブルクォーテーションを使った場合に扱えなくなる。
#@# この形式に変換することで、ダブルクォーテーションで囲んだ文字列などが扱えるようになる。

字句解析の考え方は単純です。
先頭から1文字づつ文字を読んでいき、適切な字句に振り分けていきます。
iCalendarを字句解析するために必要な字句の種類は次の5つです。

  1. 文字列
  2. コロン
  3. セミコロン
  4. イコール
  5. カンマ
  6. 終端


#@# やり方は頭から順番に文字を読んでいき、あたった文字の種類によって分岐する。
#@# 文字列なら、条件を満たす文字が続く限り読んでいく
#@# セミコロン、コロン、カンマはそれぞれで意味があるのでそのようにする。
#@# Nameを読んでいるときか、Value を読んでいるときかで条件を満たす文字が変わるのでその関数を入れ替える必要もある。


==== トークンの列からコンテンツラインに変換する

コンテンツラインに変換する。
forループでeofが来るまで回す。

途中でセミコロンが来たら、パラメーターに変換する
コロンがきたら、以降をvalueの配列に詰めていく


=== コンテンツラインをコンポーネントに変換する

==== データの型
プロパティ、パラメータのvalueには型がある
大抵はただの文字列であるtext型でOKだが、日付や回数などは特別な型にする必要がある。
型に変換することでじぜんにデータのバリデーションができる。

それぞれの型は個々にあるので、ルールに沿って変換する

プロパティによってはここからさらに特殊なルールになるものがある

==== パラメータ
パラメータの種類はここにある
それぞれのバリデーションがあるのでそれに沿ってコンテンツラインから取り出したパラメータを当てはめていく
parameter.Container という型を定義して、まとめて扱うことができるようにする。
プロパティによっては特定のパラメータを取り出す必要があるので、mapでパラメータ名から引けるようにする。
mapの要素はスライスにする。

==== プロパティのバリデーション
プロパティのバリデーションは各プロパティの仕様に沿って実装する。
プロパティの型を定義して@<code>{Setプロパティ}メソッドで値をセットするときにバリデーションする。
あるプロパティは値しか見ない場合もあるし、別のやつはparameterもチェックする場合がある。
このパラメータは指定する場合は1つしかだめとか

==== コンポーネントのバリデーション
最後にコンポーネントのバリデーションを行う。
コンポーネントのプロパティは3つの制約がある

 1. 必須コンポーネント
 2. 必須ではないが1つしか設定できないもの
 3. 0個以上設定できるもの

