= Goで入門するBurrows-Wheeler変換

こんにちは@pon@<fn>{po3rin}です。本章ではGoによる実装を通して、
@<b>{Burrows-Wheeler変換(BWT)} という文字列変換アルゴリズムを紹介します。

//footnote[po3rin][@<href>{https://twitter.com/po3rin}]

== BWTの為の事前知識

ここからはBWTを理解するための文字列界隈の事前知識をGoの実装と共に確認しましょう。

=== 辞書式順序

辞書式順序とは文字列の大小の順序の決め方です。文字列T,Uを先頭から1文字ずつ比較していき、初めて異なる文字が出てきた時に、その大小関係で順序が決まります。
Goの文字列比較では辞書式順序が採用されています(@<list>{string-order})。

//list[string-order][Goの文字列比較は辞書式順序で大小が決まる][go]{
func main() {
	if "aaab" < "aaac" {
        fmt.Println("aaab より aaacの方が大きい")
    }
}
//}

=== Suffix(接尾辞)

文字列@<code>{"T"}の任意の@<code>{"i"}番目から最後までの部分文字列を@<b>{Suffix(接尾辞)}と呼びます。
GoにはSuffixとして部分文字列が存在するかをチェックするメソッド@<code>{"func HasSuffix(s, suffix string) bool"}があります(@<list>{hassuffix})。

//list[hassuffix][GoにはSuffixに関する関数がある][go]{
func main() {
	fmt.Println(strings.HasSuffix("Amigo", "go")) // true
	fmt.Println(strings.HasSuffix("Amigo", "migo")) // true
	fmt.Println(strings.HasSuffix("Amigo", "")) //true
}
//}

=== Suffix Array

Suffixを使ったデータ構造である@<b>{Suffix Array(接尾辞配列)}を紹介します。Suffix Arrayはのちに紹介するBWTの構築にも利用されます。
文字列Tの全てのSuffixを辞書式順序でソートし、そのindexを格納した配列です。例えば@<b{abcab}>のSuffixArrayを単純に構築するときはまずSuffixのリストを作り、
それを辞書式順序でソートし、そのindexをとります(@<img>{suffixarray})。

//image[suffixarray]["abracadarba"からSuffix Arrayを構築し、文字列パターンマッチ][scale=1]{
//}

@<img>{suffixarray}からも分かるように、Suffix Arrayは文字列のパターンマッチに利用できます。Suffixをソートしているので二分探索で探索が可能です。
GoではSuffix Arrayが標準パッケージで提供されています(@<list>{suffix})。

//list[suffix][suffixarray パッケージ][go]{
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
当然、BWTは可逆変換なので、@<code>{"ard$rcaaaabb"}から@<code>{"abracadabra$"}という元の文字列に戻せます(@<list>{bwt-example})。

//list[bwt-example][BWT関数での文字列変換の例][go]{
func main() {
	t := "abracadabra"
	bwt := BWT(t) // 後ほど実装
	fmt.Println(bwt) // ard$rcaaaabb

	bwtinv := BWTInverse(bwt) // 後ほど実装
	fmt.Println(bwtinv) // abracadabra
}
//}

== BWTをGoで実装する

ではその仕組みをGoで実装しながら追っていきましょう。まずはBWT構築です。
BWTは文字列Tを構成する各文字を、それに続くSuffixをキーとして辞書式順序にソートしたものです(@<img>{suffixarray-build})。
ただし、$の後続のSuffixは元の文字列そのものとします。

//image[suffixarray-build]["abracadarba$"からBWTを構築][scale=1]{
//}

しかし、なぜこれで同じ文字が出現しやすくなるのでしょうか。各文字をそれに続くSuffixでソートするということは、
後続の文字列が似ている文字が連続で出現しやすくなるということです。
例えば、@<b>{"the"}という頻出ワードの@<b>{"t"}は後ろの@<b>{"he"}でソートされるので@<b>{"t"}は連続して出やすくなります。

それでは実際にGoでBWTを実装してみましょう。最初にSuffix Arrayを構築し、そこからBWTを構築します(@<list>{bwt-build})。

//list[bwt-build][BWT関数の実装][go]{
func BWT(t string) string {

	// Suffix Array ------------------
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

// index指定で1文字だけ取得する関数
func GetChar(t string, i int) string {
	rs := []rune(t)
	if i < 0 {
		return string(rs[len(rs)+i])
	}
	return string(rs[i])
}
//}

@<code>{GetChar}関数はstringに対して1文字ずつアクセスするための関数です。
ここで、Goのstringに対してindexアクセスする動作をもう一度復習しましょう。
Goでは文字コードをUTF-8で1byteごとに区切っています。
その為。indexアクセスではUTF-8でのbyte表現の1byteだけにアクセスしてしまいます(@<list>{byte-access})。

//list[byte-access][stringにindexアクセス][go]{
func main() {
	t:="寝たい"

	fmt.Println([]byte(t))
	// [229 175 157 227 129 159 227 129 132]

	fmt.Println(t[0])
	// 229

	fmt.Println(string(t[0]))
	// å <-寝じゃない！！
}
//}

その為、code pointを単位として文字を扱うための仕組みであるruneを単位としてアクセスすることで
@<code>{GetChar}関数では1文字アクセスを実装しています。

実装した@<code>{BWT}関数を実際に使うと正しくBWTできていることが分かります(@<list>{bwt-build-run})。

//list[bwt-build-run][BWT関数の利用例][go]{
func main() {
	b := BWT("abracadabra")
	fmt.Println(b) // ard$rcaaaabb
}
//}

今回はSuffix Arrayを構築してBWTを構築しましたが、Suffix Arrayの構築を高速化することでBWTの構築を高速に行えます。

//TODO:

== BWT文字列の復元

続いてBWTの復元です。復元に必要なのはBWT文字列だけです。これがBWTが強力な所以です。
この節ではまず逆変換に必要な LF-mapping の概念について紹介します。

=== LF-mapping

まず、辞書順に並んだ各接尾辞の先頭文字をjoinしたものをTfとすると,@<code>{"abracadabra$"}に対応するTfは@<code>{"$aaaaabbcdrr"}です(@<img>{lf})。

//image[lf]["abracadarba$"のTf][scale=1]{
//}

Tfの重要な性質として1つの文字に注目した場合、Tfに出現する順番とBWT文字列(Tbとする)に出現する順番は同じになります。
LF-mappingはTbのある文字がTfのどの文字に紐づくかをmappingするものです(@<img>{lfmapping})。

//image[lfmapping][TfとBWTの文字対応][scale=1]{
//}

BWT文字列をTbとすると、LF-mappingを行うためには、@<img>{lfmapping-eq}のように計算することでmappingできます。
配列@<m>{C[c]}はTb内で@<m>{c}より小さい文字の数です。@<m>{rank}は@<m>{Tb[:i]}の中の@<m>{c}の数を返します。
この二つの項でLF-mappingが可能です。

//image[lfmapping-eq][LF-mapping][scale=1]{
//}

例えばTbの二つ目の@<code>{b}はindexが@<code>{11}なので@<m>{LF(11)}を計算し、Tfに対応する文字の位置を特定できます(@<img>{lfmapping-ex})。

//image[lfmapping-ex][LF-mappingの使用例][scale=1]{
//}

=== LF-mappingを使ったBWT復元

LF-mappingを使ってBWTから元の文字列を復元できます。

=== BWT復元のGo実装

それではLF-mappingを使ったBWT復元をGoで実装します。

//list[bwt-inverse][BWT文字列の復元][go]{
func BWTInverse(t string) string {

	// Cの構築 ------------
	C := make(map[rune]int)
	for _, c := range t {
		v, ok := C[c]
		if !ok {
			C[c] = 1
			continue
		}
		C[c] = v + 1
	}

	sig := Deduplicate(strings.Split(t, ""))

	var sum int
	for _, c := range sig {
		r := []rune(c)[0]
		cur := C[r]
		C[r] = sum
		sum = sum + cur
	}

	// LF-mapping ----------

	psi := make(map[int]int)
	for i, c := range t {
		psi[C[c]] = i
		C[c] = C[c] + 1
	}

	// inverse -------------

	var p int
	r := []rune(t)
	result := make([]rune, len(r))
	for i := range t {
		p = psi[p]
		result[i] = r[p]
	}

	return string(result)
}
//}

== BWTを使った圧縮全文索引
