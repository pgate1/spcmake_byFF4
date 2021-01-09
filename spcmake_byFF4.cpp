/*
	spcmake_byFF4
	Copyright (c) 2020-2021 pgate1
*/

#pragma warning( disable : 4996 )
#include<stdio.h>
#include<memory.h>
#include<sys/stat.h>

#pragma warning( disable : 4786 )
#include<string>
#include<map>
#include<vector>
#include<set>
using namespace std;

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned long  uint32;

static const int FF4_BRR_NUM = 23; // 音色の数

class FF4_AkaoSoundDriver
{
public:

	uint32 driver_size;
	uint8 *driver;

	// 常駐波形
	uint16 sbrr_size;
	uint8 *sbrr;
	uint16 sbrr_start[16]; // スタートアドレスとループアドレス
	uint8 sbrr_tune[8];

	// 波形
	uint32 brr_size[FF4_BRR_NUM];
	uint8 *brr[FF4_BRR_NUM];
	uint16 brr_loop[FF4_BRR_NUM];
	uint8 brr_tune[FF4_BRR_NUM];

	uint32 attack_table_start_size;
	uint8 *attack_table_start;
	uint32 attack_table_size;
	uint8 *attack_table;

	// 効果音
	uint16 eseq_size;
	uint8 *eseq;
	uint32 eseq_start_size;
	uint8 *eseq_start;

	FF4_AkaoSoundDriver()
	{
		driver = NULL;
		attack_table_start = NULL;
		attack_table = NULL;
		int i;
		for(i=0; i<FF4_BRR_NUM; i++){
			brr[i] = NULL;
		}
		eseq = NULL;
		eseq_start = NULL;
	}

	~FF4_AkaoSoundDriver()
	{
		if(driver!=NULL) delete[] driver;
		if(attack_table_start!=NULL) delete[] attack_table_start;
		if(attack_table!=NULL) delete[] attack_table;
		int i;
		for(i=0; i<FF4_BRR_NUM; i++){
			if(brr[i]!=NULL) delete[] brr[i];
		}
		if(eseq!=NULL) delete[] eseq;
		if(eseq_start!=NULL) delete[] eseq_start;
	}

	int get_akao(const char *rom_fname);
};

// FinalFantasy4.romからAkaoサウンドドライバ等取得
int FF4_AkaoSoundDriver::get_akao(const char *rom_fname)
{
	FILE *fp = fopen(rom_fname, "rb");
	if(fp==NULL){
		printf("cant open %s.\n", rom_fname);
		return -1;
	}
	struct stat statBuf;
	int rom_size = 0;
	if(stat(rom_fname, &statBuf)==0) rom_size = statBuf.st_size;
	if(rom_size!=1024*1024){
		printf("FF4のromサイズが違う？ヘッダが付いてる？\n");
		return -1;
	}
	uint8 *rom = new uint8[1024*1024];
	fread(rom, 1, 1024*1024, fp);
	fclose(fp);

	// FF4のROMか確認
	if(rom[0]!=0x78){
		delete[] rom;
		printf("FF4のromにヘッダが付いているようです？\n");
		return -1;
	}
	if(!(rom[0x07FC0]=='F' && rom[0x07FC6]=='F' && (rom[0x07FCE]=='4' || rom[0x07FCE]=='2'))){
		delete[] rom;
		printf("FF4のromではない？\n");
		return -1;
	}

/*
0x0000 - 0x07FF　ワークメモリ
0x0800 - 0x19A8　サウンドドライバ
0x1D00 - 0x1D3F　アタックテーブルアドレス
0x1E00 - 0x1E1F　常駐波形BRRアドレス(スタート・ループ)
0x1F00 - 0x1FXX　音源BRRアドレス(スタート・ループ)
0x2000 - 0xXXXX　シーケンス
0x3000 - 0xXXXX　音源BRR（移動ok）
0xB300 - 0xCA6F　効果音シーケンス（無くてもいい）
0xCA70 - 0xCFFF　常駐波形BRR（移動ok）
0xD000 - 0xF7FF　エコーバッファ（移動ok）
0xF900 - 0xFCFF　アタックテーブル
0xFD00 - 0xFEFF　効果音シーケンスアドレス（無くてもいい）
0xFF00 - 0xFF1F　常駐波形音程補正
0xFF40 - 0xXXXX　音源音程補正
*/

	// サウンドドライバ ok
	// 0x20683 2バイトはサイズ(0x11A9)
	// 0x20685 2バイトは配置先
	// 0x20687 - 0x2182F -> 0x0800 - 0x19A8
	driver_size = *(uint16*)(rom+0x20683);
	driver = new uint8[driver_size];
	memcpy(driver, rom+0x20687, driver_size);


	// 常駐波形音程補正
	// 0x21830 2バイトはサイズ
	// 0x21832 2バイトは配置先
	// 0x21834 - 0x21853 -> 0xFF00 - 0xFF1F 32(8)byte
	memcpy(sbrr_tune, rom+0x21834, 8);

	// 常駐波形BRRアドレス
	// 0x21854 2バイトはサイズ
	// 0x21856 2バイトは配置先
	// 0x21858 - 0x21877 -> 0x1E00 - 0x1E1F (2+2)byte x 8(7)
	memcpy(sbrr_start, rom+0x21858 , 32);

	// 常駐波形BRR
	// 0x21878 2バイトはサイズ(0x590)
	// 0x2187A 2バイトは配置先
	// 0x2187C - 0x21E0B -> 0xCA70 - 0xCFFF
	sbrr_size = *(uint16*)(rom+0x21878);
	sbrr = new uint8[sbrr_size];
	memcpy(sbrr, rom+0x2187C, sbrr_size);


	// アタックテーブル
	// 0x21E0C 2バイトはサイズ(0x400)
	// 0x21E0E 2バイトは配置先
	// 0x21E10 - 0x2220F -> 0xF900 - 0xFCFF
	attack_table_size = *(uint16*)(rom+0x21E0C);
	attack_table = new uint8[attack_table_size];
	memcpy(attack_table, rom+0x21E10, attack_table_size);

	// アタックテーブルアドレス
	// 0x22210 2バイトはサイズ(0x40)
	// 0x22212 2バイトは配置先
	// 0x22214 - 0x22253 -> 0x1D00 - 0x1D3F
	attack_table_start_size = *(uint16*)(rom+0x22210);
	attack_table_start = new uint8[attack_table_start_size];
	memcpy(attack_table_start, rom+0x22214, attack_table_start_size);


	// 効果音シーケンス
	// 0x22254 2バイトはサイズ(0x1770)
	// 0x22256 2バイトは配置先
	// 0x22258 - 0x239C7 -> 0xB300 - 0xCA6F
	eseq_size = *(uint16*)(rom+0x22254);
	eseq = new uint8[eseq_size];
	memcpy(eseq, rom+0x22258, eseq_size);

	// 効果音シーケンスアドレス
	// 0x239C8 2バイトはサイズ(0x200)
	// 0x239CA 2バイトは配置先
	// 0x239CC - 0x23BCB -> 0xFD00 - 0xFEFF
	eseq_start_size = *(uint16*)(rom+0x239C8);
	eseq_start = new uint8[eseq_start_size];
	memcpy(eseq_start, rom+0x239CC, eseq_start_size);


	// 0x2000 ～ シーケンス置き場
	// 0x3000 ～ BRR置き場


	// 波形ループ位置
	// 0x248CF - 0x2492A -> 0x1F00 ～
	// リトルエンディアン4byte 最初の2byteは00 00でその後2byteがループ位置 
	int i;
	for(i=0; i<FF4_BRR_NUM; i++){
		brr_loop[i] = *(uint16*)(rom+0x248CF+i*4+2); // 4byte x 23
	}

	// 音源音程補正
	// 0x2492B - 0x24941 -> 0xFF40 ～
	memcpy(brr_tune, rom+0x2492B, FF4_BRR_NUM); // 1byte x 23

	// 音源BRR
	// 0x24942 - 0x24986 // 3byte x 23
	// リトルエンディアン3byte
	// 得た値に+0x24000した値がオフセット 最初の3byteの00はおそらく波形番号0番(無音)のこと
	for(i=0; i<FF4_BRR_NUM; i++){
		// 先頭2バイトはサイズ
		int brr_adrs = (*(uint32*)(rom+0x24942+i*3) & 0x0001FFFF) + 0x24000;
		brr_size[i] = *(uint16*)(rom+brr_adrs);
		brr[i] = new uint8[brr_size[i]];
		memcpy(brr[i], rom+brr_adrs+2, brr_size[i]);
	}
/*
{
system("mkdir brr");
int i;
for(i=0; i<FF4_BRR_NUM; i++){
	char fname[100];
	sprintf(fname, "brr/ff4_%02X.brr", i);
	FILE *fp = fopen(fname, "wb");
	fwrite(brr_loop+i, 1, 2, fp);
	fwrite(brr[i], 1, brr_size[i], fp);
	fclose(fp);
}
}
*/

	delete[] rom;

	return 0;
}

struct FF4_TONE {
	string brr_fname;
	bool used; // formatter only
	int line; // formatter only
	int brr_id; // formatter only
	int inst_id; // 常駐波形 only
	uint8 tuning; // 外部BRR用
	uint8 attack; // 0xDC formatter only
	uint8 sustain; // 0xDD formatter only
	uint8 release; // 0xDE formatter only
};

class FF4_SPC
{
public:
	string songname;
	string gametitle;
	string artist;
	string dumper;
	string comment;

	uint32 play_time;
	uint32 fade_time;

	map<int, FF4_TONE> brr_map;

	uint8 *seq[8];
	uint16 seq_size[8];

	uint16 track_loop[8]; // 0xF4 0xFFFFの場合はトラックループなし
	vector<uint16> break_point[8]; // 0xF5
	vector<uint16> jump_point[8]; // 0xF4

	uint16 brr_offset;
	bool f_brr_echo_overcheck;
	uint8 echo_depth;
	bool f_surround; // 逆位相サラウンド
	bool f_eseq_out; // 効果音シーケンス埋め込み

	FF4_SPC()
	{
		play_time = 300; // 秒、デフォルト再生時間
		fade_time = 10000; // ミリ秒、デフォルトフェードアウト時間
		int i;
		for(i=0; i<8; i++){
			seq[i] = NULL;
			seq_size[i] = 0;
			track_loop[i] = 0xFFFF;
		}
		brr_offset = 0x3000;
		f_brr_echo_overcheck = false;
		echo_depth = 5;
		f_surround = false;
		f_eseq_out = true;
	}

	~FF4_SPC()
	{
		int i;
		for(i=0; i<8; i++){
			if(seq[i]!=NULL){
				delete[] seq[i];
				seq[i] = NULL;
			}
		}
	}
};

class spcmake_byFF4
{
public:
	FF4_AkaoSoundDriver asd;
	FF4_SPC spc;
	string str;

	int read_mml(const char *mml_fname);
	int formatter(void);
	int get_sequence(void);
	int get_ticks(int track);
	int make_spc(const char *spc_fname);
};

int spcmake_byFF4::read_mml(const char *mml_fname)
{
	// シーケンスファイル読み込み
	FILE *fp = fopen(mml_fname, "r");
	if(fp==NULL){
		printf("HexMMLファイル %s が開けません.\n", mml_fname);
		return -1;
	}
	char buf[1024];
	while(fgets(buf, 1023, fp)){
		str += buf;
	}
	fclose(fp);
	return 0;
}

int line;

int skip_space(const string &str, int p)
{
	while(str[p]==' ' || str[p]=='\t' || str[p]=='\r' || str[p]=='\n'){
		if(str[p]=='\n') line++;
		p++;
	}
	return p;
}

int term_end(const string &str, int p)
{
	while(str[p]!=' ' && str[p]!='\t' && str[p]!='\r' && str[p]!='\n' && str[p]!='\0') p++;
	return p;
}

int term_begin(const string &str, int p)
{
	while(str[p]!=' ' && str[p]!='\t' && str[p]!='\r' && str[p]!='\n' && str[p]!='\0') p--;
	return p;
}

int num_end(const string &str, int p)
{
	while(isdigit(str[p])) p++;
	return p;
}

int get_hex(const string &str, int p)
{
	char buf[3];
	buf[0] = str[p];

	char c = str[p+1];
	if(!((c>='0' && c<='9') || (c>='A' && c<='F'))){
		printf("Error line %d : 16進数表記異常です.\n", line);
		getchar();
	}

	buf[1] = str[p+1];
	buf[2] = '\0';
	int hex;
	sscanf(buf, "%02x", &hex);
	return hex;
}

int is_space(const char c)
{
// isspace() 0x20 0x09 0x0D 0x0A     0x0B 0x0C
	if(c==' ' || c=='\t' || c=='\r' || c=='\n' || c=='\0'){
		return 1;
	}
	return 0;
}

int is_cmd(const char c)
{
	if((c>='0' && c<='9') || (c>='A' && c<='F')){
		return 1;
	}
	return 0;
}

int spcmake_byFF4::formatter(void)
{
	line = 1;

	// コメント削除
	int sp = 0;
	for(;;){
		sp = str.find("/*", sp);
		if(sp==string::npos) break;
		int ep = str.find("*/", sp+2);
		if(ep==string::npos) break;
		int k = 0;
		int p;
		for(p=sp; p<ep; p++) if(str[p]=='\n') k++;
		str.erase(sp, ep-sp+2);
		if(k) str.insert(sp, k, '\n');
	}
	sp = 0;
	for(;;){
		sp = str.find("//", sp);
		if(sp==string::npos) break;
		int ep = str.find('\n', sp+2);
		if(ep==string::npos) break;
		str.erase(sp, ep-sp);
	}
	
//{FILE *fp=fopen("sample_debug.txt","w");fprintf(fp,str.c_str());fclose(fp);}

	#define _MML_END_ "#track ;"
	// 最後に _MML_END_ を付加
	str.insert(str.length(), "\n" _MML_END_ " ");

	char buf[1024];
	int track_num = 0;
	map<string, FF4_TONE> tone_map;
	int brr_id = 0;
	bool f_octave_swap = false;
	bool f_auto_assign_toneid = false;
	set<string> label_set;
	map<string, int> label_map;

	int p;
	for(p=0; str[p]!='\0'; p++){
		if(str[p]=='\n'){
			line++;
		//	printf("line %d %s\n", line, str.c_str()+p+1);getchar();
		}

		if(str[p]=='#'){

			// 曲名の取得
			if(str.substr(p, 9)=="#songname"){
				int sp = str.find('"', p+9) + 1;
				int ep = str.find('"', sp);
				spc.songname = str.substr(sp, ep-sp);
				if(spc.songname.length()>32){
					printf("警告 line %d : #songnameが32バイトを超えています.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}

			// ゲーム名の取得
			if(str.substr(p, 10)=="#gametitle"){
				int sp = str.find('"', p+10) + 1;
				int ep = str.find('"', sp);
				spc.gametitle = str.substr(sp, ep-sp);
				if(spc.gametitle.length()>32){
					printf("警告 line %d : #gametitleが32バイトを超えています.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}

			// 作曲者の取得
			if(str.substr(p, 7)=="#artist"){
				int sp = str.find('"', p+7) + 1;
				int ep = str.find('"', sp);
				spc.artist = str.substr(sp, ep-sp);
				if(spc.artist.length()>32){
					printf("警告 line %d : #artistが32バイトを超えています.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}

			// 作成者の取得
			if(str.substr(p, 7)=="#dumper"){
				int sp = str.find('"', p+7) + 1;
				int ep = str.find('"', sp);
				spc.dumper = str.substr(sp, ep-sp);
				if(spc.dumper.length()>16){
					printf("警告 line %d : #dumperが16バイトを超えています.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}

			// コメントの取得
			if(str.substr(p, 8)=="#comment"){
				int sp = str.find('"', p+8) + 1;
				int ep = str.find('"', sp);
				spc.comment = str.substr(sp, ep-sp);
				if(spc.comment.length()>32){
					printf("警告 line %d : #commentが32バイトを超えています.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}

			// 再生時間とフェードアウト時間の設定
			if(str.substr(p, 7)=="#length"){
				// 再生時間
				int sp = skip_space(str, p+7);
				int ep = term_end(str, sp);
				string sec_str = str.substr(sp, ep-sp);
				int cp;
				if((cp=sec_str.find(':'))!=string::npos){ // 例：2:30
					sec_str[cp] = '\0';
					spc.play_time = atoi(sec_str.c_str()) * 60 + atoi(sec_str.c_str()+cp+1);
				}
				else{ // 例：150
					spc.play_time = atoi(sec_str.c_str());
				}

				// フェードアウト時間
				sp = skip_space(str, ep);
				ep = term_end(str, sp);
				spc.fade_time = atoi(str.substr(sp, ep-sp).c_str());
				
				str.erase(p, ep-p);
				p--;
				continue;
			}

			// BRRオフセット
			if(str.substr(p, 11)=="#brr_offset"){

				// オフセット時は効果音シーケンスは出力しない
				spc.f_eseq_out = false;

				int sp = skip_space(str, p+11);
				int ep = term_end(str, sp);
				if(str.substr(sp, ep-sp)=="auto"){
					spc.brr_offset = 0xFFFF;
				}
				else{ // 0x3000 とか
					int brr_offset = strtol(str.substr(sp, ep-sp).c_str(), NULL, 16);
				//	printf("brr_offset 0x%04X\n", brr_offset);
					if(brr_offset<0 || brr_offset>=0x10000){
						printf("Error line %d : #brr_offsetの値が不正です.\n", line);
						return -1;
					}
					spc.brr_offset = brr_offset;
				}
				str.erase(p, ep-p);
				p--;
				continue;
			}

			// BRR領域がエコーバッファに重なるのチェックを有効にする
			if(str.substr(p, 19)=="#brr_echo_overcheck"){
				spc.f_brr_echo_overcheck = true;
				str.erase(p, 19);
				p--;
				continue;
			}

			// エコー深さ指定
			if(str.substr(p, 11)=="#echo_depth"){
				int sp = skip_space(str, p+11);
				if(str[sp]=='#'){
					printf("Error : #echo_depth パラメータを指定してください.\n");
					return -1;
				}
				int ep = term_end(str, sp);
				// EDL default 5
				int echo_depth = atoi(str.substr(sp, ep-sp).c_str());
				if(echo_depth<=0 || echo_depth>=16){
					printf("Error line %d : #echo_depth は 1～15 としてください.\n", line);
					return -1;
				}
				spc.echo_depth = echo_depth;
				str.erase(p, ep-p);
				p--;
				continue;
			}

			// 逆位相サラウンド有効
			if(str.substr(p, 9)=="#surround"){
				spc.f_surround = true;
				str.erase(p, 9);
				p--;
				continue;
			}

			// オクターブコマンド入れ替え
			if(str.substr(p, 7)=="#swap<>" || str.substr(p, 7)=="#swap><"){
				f_octave_swap ^= 1;
				str.erase(p, 7);
				p--;
				continue;
			}

			// tone_idを省略時に自動割り当て
			if(str.substr(p, 19)=="#auto_assign_toneid"){
				f_auto_assign_toneid = true;
				str.erase(p, 19);
				p--;
				continue;
			}

			// 波形宣言
			if(str.substr(p, 5)=="#tone"){
				int sp = skip_space(str, p+5); // tone指定先頭
				if(str[sp]=='"'){
					// tone_id省略されている場合は内部的に割り当てる
					if(f_auto_assign_toneid){
						static int auto_id = 64;
						char id_buf[10];
						sprintf(id_buf, "%d ", auto_id++);
						str.insert(sp, id_buf);
					}
					else{
						printf("Error line %d : #tone の tone_id が指定されていません.\n", line);
						return -1;
					}
				}
				int ep = term_end(str, sp);
				string tone_id = str.substr(sp, ep-sp);
				//printf("tone_id[%s]\n",tone_id.c_str());getchar();
				if(tone_map.find(tone_id)!=tone_map.end()){
					printf("Error line %d : #tone %s はすでに宣言されています.\n", line, tone_id.c_str());
					return -1;
				}
				// 常駐波形でもオクターブ・トランスポーズ・ディチューン設定するから追加
				tone_map[tone_id].used = false; // tone追加
				tone_map[tone_id].line = line;

				// brr_fnameの取得
				sp = skip_space(str, ep) + 1;
				ep = term_end(str, sp);
				string brr_fname = str.substr(sp, ep-sp-1);
				// 常駐波形ならtrue
				bool f_stayinst = brr_fname.substr(0,9)=="FF4inst:s";
				if(brr_fname.substr(0,8)=="FF4inst:"){
					int ssp = f_stayinst ? 9 : 8;
					int eep = term_end(brr_fname, ssp);
					int inst_id = strtol(brr_fname.substr(ssp, eep-ssp).c_str(), NULL, 16);
					//printf("inst_id %d\n", inst_id);getchar();
					if(!f_stayinst && (inst_id<0 || inst_id>=FF4_BRR_NUM)){
						printf("Error line %d : FF4inst 波形指定は 00～16(16進数) としてください.\n", line);
						return -1;
					}
					if(f_stayinst && (inst_id<0 || inst_id>6)){
						printf("Error line %d : FF4inst 常駐波形指定は s0～s6 としてください.\n", line);
						return -1;
					}
					tone_map[tone_id].inst_id = inst_id;
				}
				else{
					struct stat st;
					if(stat(brr_fname.c_str(), &st)!=0){
						printf("Error line %d : BRRファイル %s がありません.\n", line, brr_fname.c_str());
						return -1;
					}
					if((st.st_size-2)%9){
						printf("Error line %d : BRRファイル %s サイズ異常です.ループアドレスが付加されていない？\n", line, brr_fname.c_str());
						return -1;
					}
				}
				// 常駐波形じゃなければBRRを追加する
				if(!f_stayinst){
// FF4で使用可能なBRR数は？
					if(brr_id>=32){
						printf("Error line %d : tone宣言(BRR指定)は32個までです.\n", line);
						return -1;
					}
					spc.brr_map[brr_id].brr_fname = brr_fname;
				}
				tone_map[tone_id].brr_fname = brr_fname;

				// パラメータ取得、#toneは一行で記述すること
				uint8 param[5];
				int param_num;
				for(param_num=0; param_num<5 && str[ep]!='\0';){
					sp = ep;
					while(str[sp]==' ' || str[sp]=='\t' || str[sp]=='\r') sp++;
					if(str[sp]=='\n') break; // num==3,4の時ここで抜ければ正常
					ep = sp;
					while(str[ep]!=' ' && str[ep]!='\t' && str[ep]!='\r' && str[ep]!='\n' && str[ep]!='\0') ep++;
					param[param_num++] = (uint8)strtol(str.substr(sp, ep-sp).c_str(), NULL, 16);
				}
				//printf("pn %d\n", param_num);

				if(brr_fname.substr(0,8)=="FF4inst:"){
					if(param_num==0){
						// 無指定
						tone_map[tone_id].attack = 0xFF; // DC DD DE を出力しない
					}
					else if(param_num==3){
						tone_map[tone_id].attack = param[0];
						tone_map[tone_id].sustain = param[1];
						tone_map[tone_id].release = param[2];
					}
					else{
						printf("Error line %d : FF4波形指定の場合のパラメータ数は0個か3個です.\n", line);
						return -1;
					}
				}
				else{ // brrファイル指定の場合
					if(param_num==4){
						spc.brr_map[brr_id].tuning = param[0];
						// adsr(DC DD DE)、パラメータは16進数
						tone_map[tone_id].attack = param[1];
						tone_map[tone_id].sustain = param[2];
						tone_map[tone_id].release = param[3];
					}
					else{
						printf("Error line %d : BRRファイル指定の場合は4個のパラメータを設定してください.\n", line);
						return -1;
					}
				}

				// 常駐波形の場合はbrr_idは使わない
				if(!f_stayinst){
					tone_map[tone_id].brr_id = brr_id;
					brr_id++;
				}

				// 削除
				str.erase(p, ep-p);
				p--;
				continue;
			}

			// トラック番号の取得
			if(str.substr(p, 6)=="#track"){

				// 前トラックの終了処理
				if(label_map.size()){
					map<string, int>::iterator mit;
					for(mit=label_map.begin(); mit!=label_map.end(); ++mit){
						if(label_set.find(mit->first)==label_set.end()){
							printf("Error track %d : トラック内でジャンプラベル %s がありません.\n", track_num, mit->first.c_str());
							return -1;
						}
					}
				}

				// #track ; → #9
				if(str.substr(p, strlen(_MML_END_))==_MML_END_){
					str.replace(p, strlen(_MML_END_), "#9");
					break;
				}

				// トラック初期化
				label_set.clear(); // ジャンプ先ラベルクリア
				label_map.clear(); // ジャンプラベルクリア

				int sp = skip_space(str, p+6);
				int ep = term_end(str, sp);
				track_num = atoi(str.substr(sp, ep-sp).c_str());
				if(!(track_num>=1 && track_num<=8)){
					printf("Error line %d : #trackナンバーは1～8としてください.\n", line);
					return -1;
				}
				sprintf(buf, "#%d", track_num);
				str.replace(p, ep-p, buf);
				p++;
				continue;
			}

			// マクロ定義
			if(str.substr(p, 6)=="#macro"){
				// マクロ定義
				int sp = skip_space(str, p+6);
				int ep = term_end(str, sp);
				string macro_key = str.substr(sp, ep-sp);
				sp = str.find('"', ep) + 1;
				ep = str.find('"', sp);
				string macro_val = str.substr(sp, ep-sp);
				//printf("macro [%s][%s]\n", macro_key.c_str(), macro_val.c_str());
				str.erase(p, ep-p+1);
				p--;
				// マクロ置換
				int lp = p;
				for(;;){
					sp = str.find(macro_key, lp);
					if(sp==string::npos) break;
					//printf("macro_val_line %d\n", line);
					ep = sp + macro_key.length();
					if(!isalnum(str[sp-1]) && !isalnum(str[ep])){
						str.replace(sp, ep-sp, macro_val);
					}
					lp = sp + macro_val.length();
				}
				continue;
			}

			printf("警告 line %d : # 未定義のコマンドです.\n", line);
			getchar();
			int ep = term_end(str, p);
			str.erase(p, ep-p);
			p--;
			continue;
		}

		// 効果音埋め込み
		if(str[p]=='@' && str[p+1]=='@'){
			int sp = p + 2;
			int ep = term_end(str, sp);
			int id = atoi(str.substr(sp, ep-sp).c_str());
			if(id<0 || id>255){
				printf("Error line %d : @@効果音番号は 0～255 までです.\n", line);
				return -1;
			}
			uint16 adrs = *(uint16*)(asd.eseq_start+id*2);
			adrs -= 0xB300;
			string estr;
			int i;
			for(i=adrs; asd.eseq[i]!=0xF1; i++){
				sprintf(buf, "%02X ", asd.eseq[i]);
				estr += buf;
			}
			str.replace(p, ep-p, estr);
			p--;
			continue;
		}

		// 波形指定の取得
		if(str[p]=='@'){ // @3 @12 @B @0C @piano
			int sp = p + 1;
			int ep = term_end(str, sp);
			string tone_id = str.substr(sp, ep-sp);
			if(tone_map.find(tone_id)==tone_map.end()){
				printf("Error line %d : @%s 定義されていません.\n", line, tone_id.c_str());
				return -1;
			}
			tone_map[tone_id].used = true;
			// FF4instでattackが0xFFなら生成しない
			char buf_attack[10], buf_sustain[10], buf_release[10];
			if(tone_map[tone_id].brr_fname.substr(0,8)=="FF4inst:" && tone_map[tone_id].attack==0xFF){
				sprintf(buf_attack, "");
				sprintf(buf_sustain, "");
				sprintf(buf_release, "");
			}
			else{
				sprintf(buf_attack, "DC %02X", tone_map[tone_id].attack);
				sprintf(buf_sustain, "DD %02X", tone_map[tone_id].sustain);
				sprintf(buf_release, "DE %02X", tone_map[tone_id].release);
			}
			sprintf(buf, "DB %02X %s %s %s ",
				(tone_map[tone_id].brr_fname.substr(0,9)=="FF4inst:s")
					? tone_map[tone_id].inst_id : (0x40 + tone_map[tone_id].brr_id),
				buf_attack, buf_sustain, buf_release
				);
			str.replace(p, ep-p, buf);
			p--;
			continue;
		}

		// ループの処理
		if(str[p]==']'){ // ループの後ろ
			int sp = skip_space(str, p+1);
			int ep = num_end(str, sp);
			int loop_count = atoi(str.substr(sp, ep-sp).c_str());
			// "]" → "F0 "
			str.replace(p, ep-p, "F0 ");
			// ループの先頭 [ を見つける
			int break_dest = 0;
			int lp = p;
			for(lp=sp; lp>=0; lp--){
				if(str[lp]=='[') break;
				// ループ中の条件ジャンプ処理
				if(str[lp]=='|'){
					str.insert(p+3, "break_dest ");
					sprintf(buf, "F5 %02X break_src ", (uint8)loop_count);
					str.replace(lp, 1, buf);
					// "|" → "F5 XX break_src " + "break_dest "
					break_dest = (16-1) + 11;
				}
			}
			if(lp==-1){
				printf("Error line %d : ] に対応する [ がありません.\n", line);
				return -1;
			}
			// "[" → "E0 02 "
			sprintf(buf, "E0 %02X ", (uint8)(loop_count-1));
			str.replace(lp, 1, buf);
			p += (6-1) + (3-1) + break_dest;
			continue;
		}

		// 10進数から16進数に変換
		if(str[p]=='d'){
			int sp = p + 1;
			int ep = term_end(str, sp);
			int d = atoi(str.substr(sp, ep-sp).c_str());
			sprintf(buf, "%02X", (uint8)d);
			str.replace(p, ep-sp+1, buf);
			p--;
			continue;
		}

		// オクターブ操作
		if(str[p]=='>'){
			if(f_octave_swap) sprintf(buf, "E2"); // オクターブ -1
			else sprintf(buf, "E1"); // オクターブ +1
			str.replace(p, 1, buf);
			p++;
			continue;
		}
		if(str[p]=='<'){
			if(f_octave_swap) sprintf(buf, "E1"); // オクターブ +1
			else sprintf(buf, "E2"); // オクターブ -1
			str.replace(p, 1, buf);
			p++;
			continue;
		}

		// トラックループ
		if(str[p]=='L' && is_space(str[p-1]) && is_space(str[p+1])){
			continue;
		}

		// ジャンプの処理
		if(str[p]=='J' && is_space(str[p-1]) && is_space(str[p+1])){
			int sp = skip_space(str, p+1);
			int ep = term_end(str, sp);
			string label_str = str.substr(sp, ep-sp);
		//	printf("[%s]\n", label_str.c_str());getchar();
			int label_num;
			if(label_map.find(label_str)==label_map.end()){
				label_num = label_map.size() + 1;
				label_map[label_str] = label_num;
			}
			else{
				label_num = label_map[label_str];
			}
			sprintf(buf, "F4 jump_src_%d%d ", track_num, label_num);
			str.replace(p, ep-p, buf);
			p += 15 -1;
		//	str.insert(p,"p");
			continue;
		}
		if(str[p]==':'){ // jump dst
			int ep = p + 1;
			int sp = term_begin(str, p-1) +1;
			string label_str = str.substr(sp, ep-sp-1);
		//	printf("[%s] %d\n", label_str.c_str(), ep-sp-1);getchar();
			if(label_set.find(label_str)!=label_set.end()){
				printf("Error line %d : ジャンプラベル %s はすでに定義されています.\n", line, label_str.c_str());
				return -1;
			}
			label_set.insert(label_str);
			int label_num;
			if(label_map.find(label_str)==label_map.end()){
				label_num = label_map.size() + 1;
				label_map[label_str] = label_num;
			}
			else{
				label_num = label_map[label_str];
			}
			sprintf(buf, "jump_dst_%d%d ", track_num, label_num);
			str.replace(sp, ep-sp, buf);
			p += -(ep-sp-1) + 12 -1;
		//	str.insert(p,"p");
			continue;
		}

		// コンバート終了
		if(str[p]=='!'){
			str.replace(p, 1, "\n" _MML_END_ " ");
			p--;
			continue;
		}
	}

	// 未使用toneの警告
	map<string, FF4_TONE>::iterator mit;
	for(mit=tone_map.begin(); mit!=tone_map.end(); ++mit){
		if(mit->second.used==false){
			printf("Warning line %d : #tone %s \"%s\" は使用されていません.\n", mit->second.line, mit->first.c_str(), mit->second.brr_fname.c_str());
		//	getchar();
		}
	}

// フォーマット処理したものをテスト出力
//FILE *fp=fopen("sample_debug.txt","w");fprintf(fp,str.c_str());fclose(fp);

	return 0;
}

int spcmake_byFF4::get_sequence(void)
{
	line = 1;

	const int SEQ_SIZE_MAX = 8*1024;
	uint8 seq[SEQ_SIZE_MAX];
	int seq_size = 0;

	int track_num = 0;
	int seq_id = -1;

	multimap<int, int> jump_src_map[8];
	map<int, int> jump_dst_map[8];

	int p;
	for(p=0; str[p]!='\0'; p++){
		if(seq_size>=SEQ_SIZE_MAX){
			printf("Error : シーケンスサイズが %d Byte を超えてしまいました.\n", SEQ_SIZE_MAX);
			return -1;
		}
		if(str[p]=='\n') line++;

		if(str[p]=='#'){
			//printf("t%d\n", track_num);
			// トラックの終了
			if(track_num!=0){
				// トラックループがあるなら
				if(spc.track_loop[seq_id]!=0xFFFF){
				//	printf("track%d_loop %d\n", track_id, spc.track_loop[track_id]);
					seq[seq_size++] = 0xF4; // ループコマンド
					// 相対値を入れておく
					seq[seq_size++] = (uint8)spc.track_loop[seq_id];
					seq[seq_size++] = spc.track_loop[seq_id] >> 8;
				//	printf("t%d seq_size %d\n", track_num, seq_size); getchar();
				}
				// F2で終わってないならF2を置く
				else if(seq_size==0 || seq[seq_size-1]!=0xF1){
					seq[seq_size++] = 0xF1;
				}

				spc.seq[seq_id] = new uint8[seq_size];
				memcpy(spc.seq[seq_id], seq, seq_size);
				spc.seq_size[seq_id] = seq_size;
				//printf("size %d\n", seq_size);
			}

			track_num = str[p+1] - '0';
			//printf("track %d\n", track_num);
			if(track_num==9) break;

			// トラックの開始
			seq_id = track_num - 1;
			if(spc.seq_size[seq_id]!=0){
				printf("Error line %d : #track トラックナンバーが重複しています.\n", line);
				return -1;
			}
			seq_size = 0;
			p++;
			continue;
		}
		if(track_num==0) continue;

		// トラックループの検出
		if(str[p]=='L'){
			if(spc.track_loop[seq_id]!=0xFFFF){ // すでに L があった
				printf("Error line %d : L の多重使用です.\n", line);
				return -1;
			}
		//	printf("t%d track_loop %d\n", track_num, seq_size);getchar();
			spc.track_loop[seq_id] = seq_size;
			continue;
		}

		// F5コマンドの処理
		// F5 01 break_src  XX XX XX F0 break_dest 
		if(seq_size>=2 && seq[seq_size-2]==0xF5 && str.substr(p, 9)=="break_src"){
			int jp = 2; // F9からの相対アドレス、2で合う
			int lp;
			for(lp=p+9; str[lp]!='#'; lp++){
				if(str.substr(lp, 10)=="break_dest"){
					str.erase(lp, 10); // break_dest削除
					// break先相対値を入れておく(必須)
					char buf[10];
					sprintf(buf, "%02X %02X ", (uint8)jp, (uint8)(jp>>8));
					str.replace(p, 9, buf); // break_src置き換え
					spc.break_point[seq_id].push_back(seq_size); // braek先アドレスを置く場所
					break;
				}
				if(is_cmd(str[lp])){
					jp++;
					lp++;
				}
			}
			p--;
			continue;
		}

		// ジャンプの前処理
		if(str.substr(p, 5)=="jump_"){
			if(str.substr(p+5, 4)=="src_"){ // jump_src_NN
				int jump_id = atoi(str.substr(p+9, 2).c_str());
				jump_src_map[seq_id].insert(pair<int, int>(jump_id, seq_size));
				str.insert(p+11, "00 00 ");
				str.erase(p, 11);
				p--;
				continue;
			}
			if(str.substr(p+5, 4)=="dst_"){ // jump_dst_NN
				int jump_id = atoi(str.substr(p+9, 2).c_str());
				jump_dst_map[seq_id][jump_id] = seq_size;
				str.erase(p, 11);
				p--;
				continue;
			}
		}

		// 制御コマンド処理
		if(is_cmd(str[p])){
			seq[seq_size++] = get_hex(str, p);
		//	printf("0x%02X ",get_hex(str, p));getchar();
			p++;
			continue;
		}
	}

	// ジャンプの後処理
	int t;
	for(t=0; t<8; t++){
		// jump_srcの位置に相対ジャンプ値を入れる
		multimap<int, int>::iterator mit;
		for(mit=jump_src_map[t].begin(); mit!=jump_src_map[t].end(); ++mit){
			// jump_rel = jump_dst - jump_src
			*(uint16*)(spc.seq[t] + mit->second) = jump_dst_map[t][mit->first] - mit->second;
			// pointにjump_srcの位置を入れる
			spc.jump_point[t].push_back(mit->second);
		}
	}

	// trackがないならシーケンス終了を置いておく
	for(t=0; t<8; t++){
		if(spc.seq_size[t]==0){
			spc.seq[t] = new uint8[1];
			spc.seq[t][0] = 0xF1;
			spc.seq_size[t] = 1;
		}
	}
/*
	{
	int t, i;
	for(t=0; t<8; t++){
		printf("track%d\n", t+1);
		for(i=0; i<spc.seq_size[t]; i++){
			printf("%02X ", spc.seq[t][i]);
		}
		printf("\n");
	}
	printf("jp %d\n", spc.break_point[0][0]);
	getchar();
	}
*/
	return 0;
}

int spcmake_byFF4::get_ticks(int track)
{
	const uint8 *seq = spc.seq[track];

	int tick = 0;

	int ticks[15] = {192, 144, 96, 72, 64, 48, 36, 32, 24, 16, 12, 8, 6, 4, 3};

	int length[48] = { // 0xD0-0xFF
		1, 1, 4, (4), 2, 3, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2,
		2, 1, 1, (1), (1), (1), 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 4, 4, 3, 4, (1), 1, 1, 1, 1, 1, 1, 1, 1, 1
	};

	int loop_depth = 0;
	int mul[10];

	int p = 0;
	for(;;){
		uint8 c = seq[p];
		if(c<=0xD1){
			int m = 1;
			for(int i=0; i<loop_depth; i++) m *= mul[i];
			tick += ticks[c % 15] * m;
			p++;
			continue;
		}
		if(c==0xE0){ // ループ開始
			mul[loop_depth++] = (uint32)seq[p+1] + 1;
		}
		else if(c==0xF5){ // 条件ジャンプ
			mul[loop_depth-1]--;
		}
		else if(c==0xF0){ // ループ終了
			loop_depth--;
		}
		else if(c==0xF4 && (p+3)==spc.seq_size[track]){ // トラックループ
			break;
		}
		else if(c==0xF1){ // 終了
			break;
		}
		p += length[c-0xD0];
	}

	return tick;
}

#include<time.h>

int spcmake_byFF4::make_spc(const char *spc_fname)
{
	// SPC Header
	uint8 header[0x100];
	memset(header, 0x00, 0x100);
	memcpy(header, "SNES-SPC700 Sound File Data v0.30", 33);
	header[0x21] = header[0x22] = header[0x23] = 26;
	header[0x24] = 30; // 0x1E

	// SPC700
	header[0x25] = 0x30; // PCL
	header[0x26] = 0x15; // PCH
//	header[0x27] = 0x00; // A
//	header[0x28] = 0x00; // X
//	header[0x29] = 0x00; // Y
//	header[0x2A] = 0x00; // PSW
	header[0x2B] = 0xCC; // SP

	{
	// バイナリフォーマット
	uint32 i;
	for(i=0; i<32 && i<spc.songname.length(); i++) header[0x2E+i] = spc.songname[i];
	for(i=0; i<32 && i<spc.gametitle.length(); i++) header[0x4E+i] = spc.gametitle[i];
	for(i=0; i<32 && i<spc.artist.length(); i++) header[0xB0+i] = spc.artist[i];
	for(i=0; i<16 && i<spc.dumper.length(); i++) header[0x6E+i] = spc.dumper[i];
	if(spc.comment.length()){
		for(i=0; i<32 && i<spc.comment.length(); i++) header[0x7E+i] = spc.comment[i];
	}
	else{
		header[0x7E] = ' '; i = 1;
	}
	}

	// SPC作成年月日(16進)
	time_t timer = time(NULL);
	struct tm *local;
	local = localtime(&timer); // 地方時に変換
	header[0x9E] = local->tm_mday; // 日
	header[0x9F] = local->tm_mon+1; // 月
	*(uint16*)(header+0xA0) = local->tm_year+1900; // 年

	// 再生時間(3byte)
	*(uint32*)(header+0xA9) = spc.play_time;
	// フェードアウト時間(4byte)
	*(uint32*)(header+0xAC) = spc.fade_time;

	// used to dump
	header[0xD1] = 4; // ETC


	// サウンドメモリ
	uint8 *ram = new uint8[0x10000];
	memset(ram, 0x00, 0x10000);
	ram[0x00F1] = 0x03; // ctrl
	ram[0x00FA] = 0x24; // target0
	ram[0x00FB] = 0x01; // target1
	ram[0x01CD] = 0x99; // スタック
	ram[0x01CE] = 0x08; // スタック
	ram[0x01CF] = 0x08; // スタック


	// DSPメモリ
	uint8 dsp_reg[128];
	memset(dsp_reg, 0x00, 128);
	dsp_reg[0x0C] = 0x40; // MVOL_L
	dsp_reg[0x1C] = spc.f_surround ? 0xC0 : 0x40; // MVOL_R
//	dsp_reg[0x0D] = 0x46; // EFB 設定できない？
	dsp_reg[0x5D] = 0x1E; // DIR
	dsp_reg[0x6C] = 0x20; // FLG
	// エコーバッファ領域設定
	uint16 echobuf_start_adrs = 0xF800 - ((uint16)spc.echo_depth << 11);
//	dsp_reg[0x6D] = echobuf_start_adrs >> 8; // ESA
//	dsp_reg[0x7D] = spc.echo_depth; // EDL

	// EDL ドライバの値変更、デフォルト0x05
	// ESAは自動計算される
	asd.driver[0x0DA1] = spc.echo_depth;


	// ベースアドレス
	*(uint16*)(ram+0x2000) = 0x2010;
	// ドライバ
	memcpy(ram+0x0800, asd.driver, asd.driver_size);
	// アタックテーブルアドレス
	memcpy(ram+0x1D00, asd.attack_table_start, asd.attack_table_start_size);
	// アタックテーブル
	memcpy(ram+0xF900, asd.attack_table, asd.attack_table_size);
	// 常駐波形音程補正
	memcpy(ram+0xFF00, asd.sbrr_tune, 8);
	// 効果音シーケンス等
	if(spc.f_eseq_out){ // オフセット調整無しなら埋め込む
		memcpy(ram+0xB300, asd.eseq, asd.eseq_size);
		memcpy(ram+0xFD00, asd.eseq_start, asd.eseq_start_size);
	}

	// 音程補正埋め込み
	uint32 i;
	for(i=0; i<spc.brr_map.size(); i++){
		if(spc.brr_map[i].brr_fname.substr(0, 8)=="FF4inst:"){
			int sp = 8;
			int ep = term_end(spc.brr_map[i].brr_fname, sp);
			int inst_id = strtol(spc.brr_map[i].brr_fname.substr(sp, ep-sp).c_str(), NULL, 16);
			ram[0xFF40+i] = asd.brr_tune[inst_id];
		}
		else{ // 外部BRR
			// 20200112 音程補正を指定できるように
			ram[0xFF40+i] = spc.brr_map[i].tuning;
		}
	}

	// 0x2010からシーケンスデータ
	uint16 seq_adrs[8];
	seq_adrs[0] = 0x2010;
	seq_adrs[1] = seq_adrs[0] + spc.seq_size[0];
	seq_adrs[2] = seq_adrs[1] + spc.seq_size[1];
	seq_adrs[3] = seq_adrs[2] + spc.seq_size[2];
	seq_adrs[4] = seq_adrs[3] + spc.seq_size[3];
	seq_adrs[5] = seq_adrs[4] + spc.seq_size[4];
	seq_adrs[6] = seq_adrs[5] + spc.seq_size[5];
	seq_adrs[7] = seq_adrs[6] + spc.seq_size[6];
	uint16 seq_adrs_end = seq_adrs[7] + spc.seq_size[7];

	// シーケンスデータを埋め込み
	for(i=0; i<8; i++){
		memcpy(ram + seq_adrs[i], spc.seq[i], spc.seq_size[i]);
	}

	// BRR位置auto
	uint16 brr_offset = spc.brr_offset; // デフォルトは0x3000
	if(spc.brr_offset==0xFFFF){
		brr_offset = seq_adrs_end;
	}
	seq_adrs_end--;
	printf("SEQ end address 0x%04X\n", seq_adrs_end); //getchar();
	if(seq_adrs_end >= spc.brr_offset){
		printf("Error : #brr_offset 0x%04X がシーケンスと重なっています.\n", spc.brr_offset);
		delete[] ram;
		return -1;
	}

	// シーケンスアドレスの埋め込み
	for(i=0; i<8; i++){
		*(uint16*)(ram+0x2000+i*2) = seq_adrs[i];
	}

	// ジャンプ制御関連
	{
	int t, i;

	// トラックループアドレス埋め込み
	// F4 XX XX
	for(t=0; t<8; t++){
	//	printf("seq%d_size %d\n", t, spc.seq_size[t]);
		if(spc.track_loop[t]!=0xFFFF){ // Lがある場合
			for(i=spc.seq_size[t]-1; i>=0; i--){
				// トラック最後のF4に対してのみループアドレスを調整
				if(spc.seq[t][i]==0xF4){
				//	printf("jump_rel %d %d\n", i, *(uint16*)(spc.seq[t]+i+1));
					uint16 jump_adrs = seq_adrs[t] + *(uint16*)(spc.seq[t]+i+1);
				//	printf("jump_adrs 0x%04X\n", jump_adrs);
					*(uint16*)(ram+seq_adrs[t]+i+1) = jump_adrs;
					break; // 20191230
				}
			}
		}
	}

	// ループブレイクジャンプ埋め込み
	// F5 NN XX XX
	for(t=0; t<8; t++){
	//	printf("seq%d_size %d\n", t, spc.seq_size[t]);
		for(i=0; i<(int)spc.break_point[t].size(); i++){
		//	printf("jump_rel %d  %d + %d\n", i, spc.break_point[t][i], *(uint16*)(spc.seq[t]+spc.break_point[t][i]));
			uint16 jump_adrs = seq_adrs[t] + spc.break_point[t][i] + *(uint16*)(spc.seq[t]+spc.break_point[t][i]);
		//	printf("jump_adrs 0x%04X\n", jump_adrs);
			*(uint16*)(ram+seq_adrs[t]+spc.break_point[t][i]) = jump_adrs;
		}
	}

	// ジャンプアドレス埋め込み
	// F4 XX XX
	for(t=0; t<8; t++){
		for(i=0; i<(int)spc.jump_point[t].size(); i++){
		//	printf("jump_rel %d  %d + %d\n", i, spc.jump_point[t][i], *(uint16*)(spc.seq[t]+spc.jump_point[t][i]));
			uint16 jump_adrs = seq_adrs[t] + spc.jump_point[t][i] + *(uint16*)(spc.seq[t]+spc.jump_point[t][i]);
		//	printf("jump_adrs 0x%04X\n", jump_adrs);
			*(uint16*)(ram+seq_adrs[t]+spc.jump_point[t][i]) = jump_adrs;
		}
	}

	}

	// BRR埋め込み
	// すでに埋め込んだBRRは使いまわす
	map<string, pair<uint16, uint16> > brr_put_map;
	uint32 adrs_index = 0;
	for(i=0; i<spc.brr_map.size(); i++){
		string brr_fname = spc.brr_map[i].brr_fname;

		uint32 start_adrs, loop_adrs;
		if(brr_put_map.find(brr_fname)==brr_put_map.end()){ // 置いてないbrrなら

			int brr_size;
			uint8 *brr_data;
			if(brr_fname.substr(0, 8)=="FF4inst:"){ // FF4の波形
				int sp = 8;
				int ep = term_end(brr_fname, sp);
				int inst_id = strtol(brr_fname.substr(sp, ep-sp).c_str(), NULL, 16);
				brr_size = asd.brr_size[inst_id] + 2; // 先頭2バイトループ追加
				brr_data = new uint8[brr_size];
				memcpy(brr_data+2, asd.brr[inst_id], asd.brr_size[inst_id]);
				*(uint16*)brr_data = asd.brr_loop[inst_id];
			//	printf("loop 0x%04X\n",asd.brr_loop[inst_id]);
			}
			else{ // BRRファイル指定
				FILE *brrfp = fopen(brr_fname.c_str(), "rb");
				if(brrfp==NULL){
					printf("Error : BRRファイル %s を開けません.\n", brr_fname.c_str());
					delete[] ram;
					return -1;
				}
				struct stat statBuf;
				if(stat(brr_fname.c_str(), &statBuf)==0) brr_size = statBuf.st_size;
			//	printf("brr_size %d\n", brr_size);
				brr_data = new uint8[brr_size];
				fread(brr_data, 1, brr_size, brrfp);
				fclose(brrfp);
			}

			start_adrs = (uint32)brr_offset + adrs_index;
			loop_adrs = start_adrs + (uint32)(*(uint16*)brr_data);
//printf("%d %s start 0x%X end 0x%X\n", 0x20+i, brr_fname.c_str(), start_adrs, start_adrs+brr_size-2-1);
			if(start_adrs+brr_size-2-1 >= 0x0F900){
				printf("BRR end address 0x%X\n", start_adrs+brr_size-2-1);
				printf("Error : %s BRRエリアが0xF900を超えました.\n", brr_fname.c_str());
				delete[] brr_data;
				delete[] ram;
				return -1;
			}
			memcpy(ram+start_adrs, brr_data+2, brr_size-2);
			delete[] brr_data;

			if(start_adrs >= 0x0F900){
				printf("BRR start address 0x%X\n", start_adrs);
				printf("Error : %s BRRスタートアドレスが0xF900を超えました.\n", brr_fname.c_str());
				delete[] ram;
				return -1;
			}
			if(loop_adrs >= 0x0F900){
				printf("BRR loop address 0x%X\n", loop_adrs);
				printf("Error : %s BRRループアドレスが0xF900を超えました.\n", brr_fname.c_str());
				delete[] ram;
				return -1;
			}

			brr_put_map[brr_fname] = pair<uint16, uint16>((uint16)start_adrs, (uint16)loop_adrs);
			adrs_index += brr_size -2;
		}
		else{ // すでに配置したbrrならアドレスを流用するだけ
			start_adrs = brr_put_map[brr_fname].first;
			loop_adrs = brr_put_map[brr_fname].second;
		}

		*(uint16*)(ram+0x1F00+i*4) = (uint16)start_adrs;
		*(uint16*)(ram+0x1F02+i*4) = (uint16)loop_adrs;
	}

	uint16 sbrr_offset;
	uint32 brr_adrs_end;
	if(spc.brr_offset==0xFFFF){ // auto
		sbrr_offset = brr_offset + (uint16)adrs_index;
		brr_adrs_end = (uint32)brr_offset + asd.sbrr_size -1;
	}
	else{ // 通常
		sbrr_offset = 0xCA70;
		brr_adrs_end = (uint32)brr_offset + adrs_index -1;
	}
	printf("BRR end address 0x%04X\n", brr_adrs_end); //getchar();
	if(spc.brr_offset!=0xFFFF && brr_adrs_end >= sbrr_offset){
		printf("Error : BRRエリアが常駐波形BRRエリアと重なっています.\n");
		return -1;
	}
	if(spc.f_eseq_out && brr_adrs_end >= 0xB300){
		printf("Error : BRRエリアが効果音シーケンスエリアと重なっています.\n");
		return -1;
	}

	// 常駐波形BRR
//	memcpy(ram+0xCA70, asd.sbrr, asd.sbrr_size);
	memcpy(ram+sbrr_offset, asd.sbrr, asd.sbrr_size);
	// 常駐波形アドレス変更
	uint16 sbrr_adrs_sa = 0xCA70 - sbrr_offset;
	for(i=0; i<7; i++){
		asd.sbrr_start[  i*2] -= sbrr_adrs_sa;
		asd.sbrr_start[1+i*2] -= sbrr_adrs_sa;
	}
	// 常駐波形BRRアドレス
	memcpy(ram+0x1E00, asd.sbrr_start, 32);

	printf("EchoBuf start   0x%04X\n", echobuf_start_adrs);//getchar();
	if(spc.f_brr_echo_overcheck){
		if((uint16)brr_adrs_end >= echobuf_start_adrs){
			printf("Error : BRRデータがエコーバッファ領域と重なっています.\n");
			delete[] ram;
			return -1;
		}
	}


	// SPC出力
	FILE *ofp;
	ofp = fopen(spc_fname, "wb");
	if(ofp==NULL){
		printf("%s cant open.\n", spc_fname);
		return -1;
	}
	fwrite(header, 1, 0x100, ofp);
	fwrite(ram, 1, 0x10000, ofp);
	fwrite(dsp_reg, 1, 128, ofp);
	fclose(ofp);
	printf("%s を生成しました.\n\n", spc_fname);
/*
	ofp = fopen("out.bin", "wb");
	if(ofp==NULL){
		printf("out.bin dont open\n");
		return -1;
	}
	fwrite(data+0x100, 1, total_size-0x100, ofp);
	fclose(ofp);
*/

	delete[] ram;

	return 0;
}

#include "brr2wav.cpp"

int main(int argc, const char *argv[])
{
	printf("[ spcmake_byFF4 ver.20210109 ]\n\n");

#ifdef _DEBUG
	argc = 5;
	argv[1] = "-i";
	argv[2] = "sample.txt";
	argv[3] = "-o";
	argv[4] = "sample.spc";
#endif

	if(argc<5){
		printf("usage : spcmake_byFF4.exe -i input.txt -o output.spc\n");
		getchar();
		return -1;
	}

	const char *input_fname = NULL;
	const char *output_fname = NULL;
	bool f_ticks = false;
	bool f_brr2wav = false;

	int argi;
	for(argi=1; argi<argc; argi++){
		if(strncmp(argv[argi], "-i", 2)==0){
			input_fname = argv[argi+1];
			argi++;
		}
		else if(strncmp(argv[argi], "-o", 2)==0){
			output_fname = argv[argi+1];
			if(strncmp(output_fname+strlen(output_fname)-4, ".spc", 4)!=0){
				printf("Error : -o %s 出力ファイル名が.spcではありません.\n", output_fname);
				return -1;
			}
			argi++;
		}
		else if(strncmp(argv[argi], "-ticks", 6)==0){
			f_ticks = true;
		}
		else if(strncmp(argv[argi], "-brr2wav", 8)==0){
			f_brr2wav = true;
		}
		else{
			printf("不明なオプション？ %s \n", argv[argi]);
			getchar();
			return -1;
		}
	}

	spcmake_byFF4 spcmakeff4;

	if(spcmakeff4.asd.get_akao("FinalFantasy4.rom")){
		return -1;
	}

	if(f_brr2wav){
		system("mkdir brr2wav");
		int i;
		for(i=0; i<FF4_BRR_NUM; i++){
			char wav_fname[40];
			sprintf(wav_fname, "brr2wav/FF4_%02X.wav", i);
			brr2wav(wav_fname, spcmakeff4.asd.brr[i], spcmakeff4.asd.brr_size[i], spcmakeff4.asd.brr_loop[i], 0x1000);
		}
	}

	if(spcmakeff4.read_mml(input_fname)){
		printf("\n");
		return -1;
	}

	if(spcmakeff4.formatter()){
		printf("\n");
#ifdef _DEBUG
	getchar();
#endif
		return -1;
	}

//fp=fopen("out.txt","w");fprintf(fp,str.c_str());fclose(fp);
	printf("\n");
	printf("songname[%s]\n", spcmakeff4.spc.songname.c_str());
	printf("dumper[%s]\n", spcmakeff4.spc.dumper.c_str());
	printf("\n");

	if(spcmakeff4.get_sequence()){
		printf("\n");
		return -1;
	}

	if(spcmakeff4.make_spc(output_fname)){
		printf("\n");
		return -1;
	}

	if(f_ticks){
		printf("\n");
		int i;
		for(i=0; i<8; i++){
			printf("  track%d %6d ticks\n", i+1, spcmakeff4.get_ticks(i));
		}
		getchar();
	}

//	getchar();

	return 0;
}
