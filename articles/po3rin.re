= Goで実装するBurrows-Wheeler変換、そして圧縮全文索引

こんにちは@pon@<fn>{po3rin}です。本章ではGoによる実装を通して、
@<b>{Burrows-Wheeler変換(BWT)} という文字列変換アルゴリズムを紹介します。

//footnote[po3rin][@<href>{https://twitter.com/po3rin}]

== BWTの為の事前知識

ここからはBWTを理解するための文字列界隈の事前知識をGoの実装と共に確認しましょう。

=== 辞書式順序

辞書式順序とは文字列の大小の順序の決め方です。文字列T,Uを先頭から1文字ずつ比較していき、初めて異なる文字が出てきた時に、その大小関係で順序が決まります。
Goの文字列比較では辞書式順序が採用されています(@<list>{string-order})。

//list[string-order][string-order][go]{
func main() {
	if "aaab" < "aaac" {
        fmt.Println("aaab より aaacの方が大きい")
    }
}
//}

=== 接尾辞

文字列@<code>{"T"}の任意の@<code>{"i"}番目から最後までの部分文字列を@<b>{接尾辞(Suffix)}と呼びます。
GoにはSuffixとして部分文字列が存在するかをチェックするメソッド@<code>{"func HasSuffix(s, suffix string) bool"}があります(@<list>{suffix})。

//list[suffix][suffix][go]{
func main() {
	fmt.Println(strings.HasSuffix("Amigo", "go")) // true
    fmt.Println(strings.HasSuffix("Amigo", "migo")) // true
	fmt.Println(strings.HasSuffix("Amigo", "")) //true
}
//}

=== Suffix Array

接尾辞を使ったデータ構造である@<b>{接尾辞配列(Suffix Array)}を紹介します。Suffix Arrayはのちに紹介するBWTの構築にも利用されます。
文字列Tの全ての接尾辞を辞書式順序でソートし、そのindexを格納した配列です。例えば@<b{abcab}>のSuffixArrayを単純に構築するときはまず接尾辞のリストを作り、
それを辞書式順序でソートし、そのindexをとります(@<img>{suffixarray})。

//image[suffixarray]["abracadarba"からSuffix Arrayを構築し、文字列パターンマッチ][scale=1]{
//}

@<img>{suffixarray}からも分かるように、Suffix Arrayは文字列のパターンマッチに利用できます。接尾辞をソートしているので二分探索で探索が可能です。
GoではSuffix Arrayが標準パッケージで提供されています。

//list[suffix][suffix][go]{
import (
    suffixarray
    // ...
)

func Example() {
	t := "abracadabra"
	sa := suffixarray.New([]byte(t))

    // abが出現する位置を返す。
	offsets := index.Lookup([]byte("ab"), -1)
    fmt.Println(offsets) // [7, 0]
}
//}

GoのsuffixarrayパッケージではSuffix Arrayの構築を@<b>{SAIS}というアルゴリズムを利用しています。
SAISによるSuffix Array構築の計算量は O(n) に抑えられます。残念ながらGoのSuffix Arrayはパターンマッチングに用途が特化している為、BWT構築には使えません。

== Burrows-Wheeler変換

それではいよいよ本題です。@<b>{Burrows-Wheeler変換(BWT)}はブロックソートとも呼ばれ、文字列の可逆変換のアルゴリズムです。
BWTはもともと、データ圧縮などの為に開発されたアルゴリズムですが、その後にその有用性が広まり、全文検索などでも利用されています。

実際にBWTはデータ圧縮プログラムであるbzip2の内部のアルゴリズムとしても採用されており、文字列を圧縮しやすい順番に文字を可逆変換します。

例えば@<code>{"abracadabra"}という文字列からBWTを構築する場合をみていきます。まずマーカーとなる文字@<code>{"$"}(他には現れない、他の文字より)を末尾に追加し、
@<code>{"abracadabra$"}という形にした後、BWTすると@<code>{"ard$rcaaaabb"}という同じ文字が連続で並ぶ圧縮しやすい形になります。
当然、BWTは可逆変換なので、@<code>{"ard$rcaaaabb"}から@<code>{"abracadabra"}という元の文字列に戻せます(@<list>{bwt-example})。

//list[bwt-example][bwt-example][go]{
func main() {
    t := "abracadabra"
	bwt := BWT(t) // 後ほど実装
    fmt.Println(bwt) // ard$rcaaaabb

    bwtinv := BWTInverse(bwt) // 後ほど実装
    fmt.Println(bwtinv) // abracadabra
}
//}

== BWTをGoで実装する

ではその仕組みをGoで実装しながら追っていきましょう。まずはBWT構築です。構築には前節で紹介したSuffix Arrayを利用します。

//list[bwt-build][bwt-build][go]{
func BWT(t string) string {
    // Suffix Array を構築 ------------------
	t += "$"
	sa := make([]string, len(t))
	for i := 0; i < len(t); i++ {
		sa[i] = t[i:]
	}
	sort.Strings(sa)

    // Suffix Array を利用してBWTを構築 -------
	var result string
	for _, v := range sa {
		if len(v) < len(t) {
			result += GetChar(t, -len(v)-1)
			continue
		}
		result += GetChar(t, -1)
	}
	return result
}
//}

//list[bwt-build-run][bwt-build-run][go]{
func main() {
	b := BWT("Go is expressive, concise, clean, and efficient. Its concurrency mechanisms make it easy to write programs that get the most out of multicore and networked machines, while its novel type system enables flexible and modular program construction. Go compiles quickly to machine code yet has the convenience of garbage collection and the power of run-time reflection. It's a fast, statically typed, compiled language that feels like a dynamically typed, interpreted language.")
	fmt.Println(b)
	s := BWTInverse(b)
	fmt.Println(s)
}
//}

== BWT文字列の復元

== ランレングス圧縮でBWTの威力をみる

== BWTを使った圧縮全文索引
