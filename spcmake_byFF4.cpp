/*
	spcmake_byFF4
	Copyright (c) 2020 pgate1
*/

#include<stdio.h>
#include<memory.h>
#include<sys/stat.h>

#pragma warning( disable : 4786 )
#include<string>
#include<map>
#include<vector>
using namespace std;

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned long  uint32;

class FF4_AkaoSoundDriver
{
public:
	uint32 driver_size;
	uint8 *driver;

	// �풓�g�`
	uint32 sbrr_size;
	uint8 *sbrr;
	uint16 sbrr_start[16]; // �X�^�[�g�A�h���X�ƃ��[�v�A�h���X
	uint8 sbrr_tune[8];

	// �g�`
	uint32 brr_size[23];
	uint8 *brr[23];
	uint16 brr_loop[23];
	uint8 brr_tune[23];

	uint32 attack_table_start_size;
	uint8 *attack_table_start;
	uint32 attack_table_size;
	uint8 *attack_table;

	FF4_AkaoSoundDriver()
	{
		driver = NULL;
		attack_table_start = NULL;
		attack_table = NULL;
		int i;
		for(i=0; i<23; i++){
			brr[i] = NULL;
		}
	}
	~FF4_AkaoSoundDriver()
	{
		if(driver!=NULL) delete[] driver;
		if(attack_table_start!=NULL) delete[] attack_table_start;
		if(attack_table!=NULL) delete[] attack_table;
		int i;
		for(i=0; i<23; i++){
			if(brr[i]!=NULL) delete[] brr[i];
		}
	}
};

// FinalFantasy4.rom����Akao�T�E���h�h���C�o���擾
int get_akao(const char *fname, FF4_AkaoSoundDriver &asd)
{
	FILE *fp = fopen(fname, "rb");
	if(fp==NULL){
		printf("cant open %s.\n", fname);
		return -1;
	}
	struct stat statBuf;
	int rom_size = 0;
	if(stat(fname, &statBuf)==0) rom_size = statBuf.st_size;
	if(rom_size!=1024*1024){
		printf("FF4��rom�T�C�Y���Ⴄ�H�w�b�_���t���Ă�H\n");
		return -1;
	}
	uint8 *rom = new uint8[1024*1024];
	fread(rom, 1, 1024*1024, fp);
	fclose(fp);

	// FF4��ROM���m�F
	if(rom[0]!=0x78){
		delete[] rom;
		printf("FF4��rom�Ƀw�b�_���t���Ă���悤�ł��H\n");
		return -1;
	}
	if(!(rom[0x07FC0]=='F' && rom[0x07FC6]=='F' && rom[0x07FCE]=='4')){
		delete[] rom;
		printf("FF4��rom�ł͂Ȃ��H\n");
		return -1;
	}

/*
0x0000 - 0x07FF�@���[�N������
0x0800 - 0x19A8�@�T�E���h�h���C�o
0x1D00 - 0x1D3F�@�A�^�b�N�e�[�u���A�h���X
0x1E00 - 0x1E1F�@�풓�g�`BRR�A�h���X(�X�^�[�g�E���[�v)
0x1F00 - 0x1FXX�@����BRR�A�h���X(�X�^�[�g�E���[�v)
0x2000 - 0xXXXX�@�V�[�P���X
0x3000 - 0xXXXX�@����BRR�i�ړ�ok�j
0xB300 - 0xCA6F�@���ʉ��V�[�P���X�i�����Ă������j
0xCA70 - 0xCFFF�@�풓�g�`BRR�i�ړ�ok�j
0xD000 - 0xF7FF�@�G�R�[�o�b�t�@�i�ړ�ok�j
0xF900 - 0xFCFF�@�A�^�b�N�e�[�u��
0xFD00 - 0xFEFF�@���ʉ��V�[�P���X�A�h���X�i�����Ă������j
0xFF00 - 0xFF1F�@�풓�g�`�����␳
0xFF40 - 0xXXXX�@���������␳
*/

	// �T�E���h�h���C�o ok
	// 0x20683 2�o�C�g�̓T�C�Y(0x11A9)
	// 0x20685 2�o�C�g�͔z�u��
	// 0x20687 - 0x2182F -> 0x0800 - 0x19A8
	asd.driver_size = *(uint16*)(rom+0x20683);
	asd.driver = new uint8[asd.driver_size];
	memcpy(asd.driver, rom+0x20687, asd.driver_size);


	// �풓�g�`�����␳
	// 0x21830 2�o�C�g�̓T�C�Y
	// 0x21832 2�o�C�g�͔z�u��
	// 0x21834 - 0x21853 -> 0xFF00 - 0xFF1F 32(8)byte
	memcpy(asd.sbrr_tune, rom+0x21834, 8);

	// �풓�g�`BRR�A�h���X
	// 0x21854 2�o�C�g�̓T�C�Y
	// 0x21856 2�o�C�g�͔z�u��
	// 0x21858 - 0x21877 -> 0x1E00 - 0x1E1F (2+2)byte x 8(7)
	memcpy(asd.sbrr_start, rom+0x21858 , 32);

	// �풓�g�`BRR
	// 0x21878 2�o�C�g�̓T�C�Y(0x590)
	// 0x2187A 2�o�C�g�͔z�u��
	// 0x2187C - 0x21E0B -> 0xCA70 - 0xCFFF
	asd.sbrr_size = *(uint16*)(rom+0x21878);
	asd.sbrr = new uint8[asd.sbrr_size];
	memcpy(asd.sbrr, rom+0x2187C, asd.sbrr_size);


	// �A�^�b�N�e�[�u��
	// 0x21E0C 2�o�C�g�̓T�C�Y(0x400)
	// 0x21E0E 2�o�C�g�͔z�u��
	// 0x21E10 - 0x2220F -> 0xF900 - 0xFCFF
	asd.attack_table_size = *(uint16*)(rom+0x21E0C);
	asd.attack_table = new uint8[asd.attack_table_size];
	memcpy(asd.attack_table, rom+0x21E10, asd.attack_table_size);

	// �A�^�b�N�e�[�u���A�h���X
	// 0x22210 2�o�C�g�̓T�C�Y(0x40)
	// 0x22212 2�o�C�g�͔z�u��
	// 0x22214 - 0x22253 -> 0x1D00 - 0x1D3F
	asd.attack_table_start_size = *(uint16*)(rom+0x22210);
	asd.attack_table_start = new uint8[asd.attack_table_start_size];
	memcpy(asd.attack_table_start, rom+0x22214, asd.attack_table_start_size);


	// ���ʉ��V�[�P���X
	// 0x22254 2�o�C�g�̓T�C�Y(0x1770)
	// 0x22256 2�o�C�g�͔z�u��
	// 0x22258 - 0x239C7 -> 0xB300 - 0xCA6F

	// ���ʉ��V�[�P���X�A�h���X
	// 0x239C8 2�o�C�g�̓T�C�Y(0x200)
	// 0x239CA 2�o�C�g�͔z�u��
	// 0x239CC - 0x23BCB -> 0xFD00 - 0xFEFF


	// 0x2000 �` �V�[�P���X�u����
	// 0x3000 �` BRR�u����

	// �g�`���[�v�ʒu
	// 0x248CF - 0x2492A -> 0x1F00 �`
	// ���g���G���f�B�A��4byte �ŏ���2byte��00 00�ł��̌�2byte�����[�v�ʒu 
	int i;
	for(i=0; i<23; i++){
		asd.brr_loop[i] = *(uint16*)(rom+0x248CF+i*4+2); // 4byte x 23
	}

	// ���������␳
	// 0x2492B - 0x24941 -> 0xFF40 �`
	memcpy(asd.brr_tune, rom+0x2492B, 23); // 1byte x 23

	// ����BRR
	// 0x24942 - 0x24989 // 3byte x 23
	// ���g���G���f�B�A��3byte
	// �����l��+0x24000�����l���I�t�Z�b�g �ŏ���3byte��00�͂����炭�g�`�ԍ�0��(����)�̂���
	for(i=0; i<23; i++){
		// �擪2�o�C�g�̓T�C�Y
		int brr_adrs = (*(uint32*)(rom+0x24942+i*3) & 0x00FFFFFF) + 0x24000;
		asd.brr_size[i] = *(uint16*)(rom+brr_adrs);
		asd.brr[i] = new uint8[asd.brr_size[i]];
		memcpy(asd.brr[i], rom+brr_adrs+2, asd.brr_size[i]);
	}
/*
{
system("mkdir brr");
int i;
for(i=0; i<23; i++){
	char fname[100];
	sprintf(fname, "brr/ff4_%02X.brr", i);
	FILE *fp = fopen(fname, "wb");
	fwrite(asd.brr_loop+i, 1, 2, fp);
	fwrite(asd.brr[i], 1, asd.brr_size[i], fp);
	fclose(fp);
}
}
*/

	delete[] rom;

	return 0;
}

struct TONE {
	string brr_fname;
	int brr_id; // formatter only
	int inst_id; // �풓�g�` only
	uint8 attack; // 0xDC formatter only
	uint8 sustain; // 0xDD formatter only
	uint8 release; // 0xDE formatter only
};

class SPC
{
public:
	string songname;
	string gametitle;
	string artist;
	string dumper;
	string comment;

	uint32 play_time;
	uint32 fade_time;

	map<int, TONE> brr_map;

	uint8 *seq[8];
	uint32 seq_size[8];

	uint16 track_loop[8]; // 0xF4 0xFFFF�̏ꍇ�̓g���b�N���[�v�Ȃ�
	vector<uint16> break_point[8]; // 0xF5

	uint16 brr_offset;
	bool f_brr_echo_overcheck;
	uint16 echo_depth;
	bool f_surround; // �t�ʑ��T���E���h

	SPC()
	{
		play_time = 300; // �b�A�f�t�H���g�Đ�����
		fade_time = 10000; // �~���b�A�f�t�H���g�t�F�[�h�A�E�g����
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
	}
	~SPC()
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

int get_hex(const string &str, int p)
{
	char buf[3];
	buf[0] = str[p];

	char c = str[p+1];
	if(!((c>='0' && c<='9') || (c>='A' && c<='F'))){
		printf("Error line %d : 16�i���\�L�ُ�ł�.\n", line);
		getchar();
	}

	buf[1] = str[p+1];
	buf[2] = '\0';
	int hex;
	sscanf(buf, "%02x", &hex);
	return hex;
}

int is_cmd(const char c)
{
	if((c>='0' && c<='9') || (c>='A' && c<='F')){
		return 1;
	}
	return 0;
}


int formatter(string &str, FF4_AkaoSoundDriver &asd, SPC &spc)
{
	line = 1;

	// �R�����g�폜
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

	char buf[1024];
	map<int, TONE> tone_map;
	int brr_id = 0;

	int p;
	for(p=0; str[p]!='\0'; p++){
		if(str[p]=='\n'){
			line++;
		//	printf("line %d %s\n", line, str.c_str()+p+1);getchar();
		}
		if(str[p]=='#'){
			// �Ȗ��̎擾
			if(str.substr(p, 9)=="#songname"){
				int sp = str.find('"', p+9) + 1;
				int ep = str.find('"', sp);
				spc.songname = str.substr(sp, ep-sp);
				if(spc.songname.length()>32){
					printf("�x�� line %d : #songname��32�o�C�g�𒴂��Ă��܂�.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}
			// �Q�[�����̎擾
			if(str.substr(p, 10)=="#gametitle"){
				int sp = str.find('"', p+10) + 1;
				int ep = str.find('"', sp);
				spc.gametitle = str.substr(sp, ep-sp);
				if(spc.gametitle.length()>32){
					printf("�x�� line %d : #gametitle��32�o�C�g�𒴂��Ă��܂�.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}
			// ��Ȏ҂̎擾
			if(str.substr(p, 7)=="#artist"){
				int sp = str.find('"', p+7) + 1;
				int ep = str.find('"', sp);
				spc.artist = str.substr(sp, ep-sp);
				if(spc.artist.length()>32){
					printf("�x�� line %d : #artist��32�o�C�g�𒴂��Ă��܂�.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}
			// �쐬�҂̎擾
			if(str.substr(p, 7)=="#dumper"){
				int sp = str.find('"', p+7) + 1;
				int ep = str.find('"', sp);
				spc.dumper = str.substr(sp, ep-sp);
				if(spc.dumper.length()>16){
					printf("�x�� line %d : #dumper��16�o�C�g�𒴂��Ă��܂�.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}
			// �R�����g�̎擾
			if(str.substr(p, 8)=="#comment"){
				int sp = str.find('"', p+8) + 1;
				int ep = str.find('"', sp);
				spc.comment = str.substr(sp, ep-sp);
				if(spc.comment.length()>32){
					printf("�x�� line %d : #comment��32�o�C�g�𒴂��Ă��܂�.\n", line);
					getchar();
				}
				str.erase(p, ep-p+1);
				p--;
				continue;
			}
			// �Đ����Ԃƃt�F�[�h�A�E�g���Ԃ̐ݒ�
			if(str.substr(p, 7)=="#length"){
				int sp = skip_space(str, p+7);
				int ep = term_end(str, sp);
				string sec_str = str.substr(sp, ep-sp);
				int cp;
				if((cp=sec_str.find(':'))!=string::npos){ // ��F2:30
					sec_str[cp] = '\0';
					spc.play_time = atoi(sec_str.c_str()) * 60 + atoi(sec_str.c_str()+cp+1);
				}
				else{ // ��F150
					spc.play_time = atoi(sec_str.c_str());
				}

				// �t�F�[�h�A�E�g����
				sp = skip_space(str, ep);
				ep = term_end(str, sp);
				spc.fade_time = atoi(str.substr(sp, ep-sp).c_str());
				
				str.erase(p, ep-p);
				p--;
				continue;
			}
			// BRR�I�t�Z�b�g
			if(str.substr(p, 11)=="#brr_offset"){
				int sp = skip_space(str, p+11);
				int ep = term_end(str, sp);
				if(str.substr(sp, ep-sp)=="auto"){
					spc.brr_offset = 0xFFFF;
				}
				else{ // 0x3000 �Ƃ�
					int brr_offset = strtol(str.substr(sp, ep-sp).c_str(), NULL, 16);
				//	printf("brr_offset 0x%04X\n", brr_offset);
					if(brr_offset<0 || brr_offset>=0x10000){
						printf("Error line %d : #brr_offset�̒l���s���ł�.\n", line);
						return -1;
					}
					spc.brr_offset = brr_offset;
				}
				str.erase(p, ep-p);
				p--;
				continue;
			}
			// BRR�̈悪�G�R�[�o�b�t�@�ɏd�Ȃ�̃`�F�b�N��L���ɂ���
			if(str.substr(p, 19)=="#brr_echo_overcheck"){
				spc.f_brr_echo_overcheck = true;
				int ep = term_end(str, p);
				str.erase(p, ep-p);
				p--;
				continue;
			}
			/*
			// �G�R�[�[���w��
			if(str.substr(p, 11)=="#echo_depth"){
				int sp = skip_space(str, p+11);
				if(str[sp]=='#'){
					printf("Error : #echo_depth �p�����[�^���w�肵�Ă�������.\n");
					return -1;
				}
				int ep = term_end(str, sp);
				// EDL default 5
				int echo_depth = atoi(str.substr(sp, ep-sp).c_str());
				if(echo_depth<=0 || echo_depth>=16){
					printf("Error line %d : #echo_depth �� 1�`15 �Ƃ��Ă�������.\n", line);
					return -1;
				}
				spc.echo_depth = echo_depth;
				str.erase(p, ep-p);
				p--;
				continue;
			}
			*/
			// �t�ʑ��T���E���h�L��
			if(str.substr(p, 9)=="#surround"){
				spc.f_surround = true;
				int ep = term_end(str, p);
				str.erase(p, ep-p);
				p--;
				continue;
			}
			// �g�`�錾
			if(str.substr(p, 5)=="#tone"){
				int sp = skip_space(str, p+5); // �����̐擪
				int ep = term_end(str, sp);
				int tone_num = atoi(str.substr(sp, ep-sp).c_str());
				if(tone_map.find(tone_num)!=tone_map.end()){
					printf("Error line %d : #tone %d �͂��łɐ錾����Ă��܂�.\n", line, tone_num);
					return -1;
				}
				// �풓�g�`�ł��I�N�^�[�u�E�g�����X�|�[�Y�E�f�B�`���[���ݒ肷�邩��ǉ�
				tone_map[tone_num]; // tone�ǉ�

				// brr_fname�̎擾
				sp = skip_space(str, ep) + 1;
				ep = term_end(str, sp);
				string brr_fname = str.substr(sp, ep-sp-1);
				// �풓�g�`�Ȃ�true
				bool f_stayinst = brr_fname.substr(0,9)=="FF4inst:s";
				if(brr_fname.substr(0,8)=="FF4inst:"){
					int ssp = f_stayinst ? 9 : 8;
					int eep = term_end(brr_fname, ssp);
					int inst_id = atoi(brr_fname.substr(ssp, eep-ssp).c_str());
					if(!f_stayinst && (inst_id<0 || inst_id>22)){
						printf("Error line %d : FF4inst �g�`�w��� 0�`22 �Ƃ��Ă�������.\n", line);
						return -1;
					}
					if(f_stayinst && (inst_id<0 || inst_id>6)){
						printf("Error line %d : FF4inst �풓�g�`�w��� s0�`s6 �Ƃ��Ă�������.\n", line);
						return -1;
					}
					tone_map[tone_num].inst_id = inst_id;
				}
				else{
					struct stat st;
					if(stat(brr_fname.c_str(), &st)!=0){
						printf("Error line %d : BRR�t�@�C�� %s ������܂���.\n", line, brr_fname.c_str());
						return -1;
					}
				}
				// �풓�g�`����Ȃ����BRR��ǉ�����
				if(!f_stayinst){
// FF4�Ŏg�p�\��BRR���́H
					if(brr_id>=32){
						printf("Error line %d : BRR��32�܂łł�.\n", line);
						return -1;
					}
					spc.brr_map[brr_id].brr_fname = brr_fname;
				}
				tone_map[tone_num].brr_fname = brr_fname;

				// �p�����[�^�擾�A#tone�͈�s�ŋL�q���邱��
				uint8 param[4];
				int param_num;
				for(param_num=0; param_num<4 && str[ep]!='\0';){
					sp = ep;
					while(str[sp]==' ' || str[sp]=='\t' || str[sp]=='\r') sp++;
					if(str[sp]=='\n') break; // num==3�̎������Ŕ�����ΐ���
					ep = sp;
					while(str[ep]!=' ' && str[ep]!='\t' && str[ep]!='\r' && str[ep]!='\n' && str[ep]!='\0') ep++;
					param[param_num++] = strtol(str.substr(sp, ep-sp).c_str(), NULL, 16);
				}
				//printf("pn %d\n", param_num);

				if(brr_fname.substr(0,8)=="FF4inst:"){
					if(param_num==0){
						// ���w��
						tone_map[tone_num].attack = 0xFF; // DC DD DE ���o�͂��Ȃ�
					}
					else if(param_num==3){
						tone_map[tone_num].attack = param[0];
						tone_map[tone_num].sustain = param[1];
						tone_map[tone_num].release = param[2];
					}
					else{
						printf("Error line %d : FF4�g�`�w��̏ꍇ�̃p�����[�^����0��3�ł�.\n", line);
						return -1;
					}
				}
				else{ // brr�t�@�C���w��̏ꍇ
					if(param_num==3){
						// adsr(DC DD DE)�A�p�����[�^��16�i��
						tone_map[tone_num].attack = param[0];
						tone_map[tone_num].sustain = param[1];
						tone_map[tone_num].release = param[2];
					}
					else{
						printf("Error line %d : BRR�t�@�C���w��̏ꍇ��3�̃p�����[�^��ݒ肵�Ă�������.\n", line);
						return -1;
					}
				}

				// �풓�g�`�̏ꍇ��brr_id�͎g��Ȃ�
				if(!f_stayinst){
					tone_map[tone_num].brr_id = brr_id;
					brr_id++;
				}

				// �폜
				str.erase(p, ep-p);
				p--;
				continue;
			}
			// �g���b�N�ԍ��̎擾
			if(str.substr(p, 6)=="#track"){
				int sp = skip_space(str, p+6);
				int ep = term_end(str, sp);
				int track_num = atoi(str.substr(sp, ep-sp).c_str());
				if(!(track_num>=1 && track_num<=8)){
					printf("Error line %d : #track�i���o�[��1�`8�Ƃ��Ă�������.\n", line);
					return -1;
				}
				sprintf(buf, "#%d", track_num);
				str.replace(p, ep-p, buf);
				p++;
				continue;
			}
			// �}�N����`
			if(str.substr(p, 6)=="#macro"){
				// �}�N����`
				int sp = skip_space(str, p+6);
				int ep = term_end(str, sp);
				string macro_key = str.substr(sp, ep-sp);
				sp = str.find('"', ep) + 1;
				ep = str.find('"', sp);
				string macro_val = str.substr(sp, ep-sp);
				//printf("macro [%s][%s]\n", macro_key.c_str(), macro_val.c_str());
				str.erase(p, ep-p+1);
				p--;
				// �}�N���u��
				int lp = p;
				for(;;){
					sp = str.find(macro_key, lp);
					if(sp==string::npos) break;
					//printf("macro_val_line %d\n", line);
					ep = sp + macro_key.length();
					if((!isalnum(str[sp-1])) && (!isalnum(str[ep]))){
						str.replace(sp, ep-sp, macro_val);
					}
					lp = sp + macro_val.length();
				}
				continue;
			}

			printf("�x�� line %d : # ����`�̃R�}���h�ł�.\n", line);
			getchar();
			int ep = term_end(str, p);
			str.erase(p, ep-p);
			p--;
			continue;
		}
		/*
		// ���ʉ����ߍ���
		if(str[p]=='@' && str[p+1]=='@'){
			int sp = p + 2;
			int ep = term_end(str, sp);
			int id = atoi(str.substr(sp, ep-sp).c_str());
			if(id<0 || id>353){
				printf("Error line %d : @@���ʉ��ԍ��� 0�`353 �܂łł�.\n", line);
				return -1;
			}
			uint16 adrs = *(uint16*)(asd.eseq+id*2);
			if(adrs==0x0000){
				printf("Error line %d : @@%d ���ʉ��͂���܂���.\n", line, id);
				return -1;
			}
			adrs -= 0x2C00;
			string estr;
			int i;
			for(i=adrs; asd.eseq[i]!=0xF2; i++){
				sprintf(buf, "%02X ", asd.eseq[i]);
				estr += buf;
			}
			str.replace(p, ep-p, estr);
			p--;
			continue;
		}
		*/
		// �g�`�w��̎擾
		if(str[p]=='@'){ // @3 @12
			int sp = p + 1;
			int ep = term_end(str, sp);
			int tone_num = atoi(str.substr(sp, ep-sp).c_str());
			if(tone_map.find(tone_num)==tone_map.end()){
				printf("Error line %d : @%d ��`����Ă��܂���.\n", line, tone_num);
				return -1;
			}
			// FF4inst��attack��0xFF�Ȃ琶�����Ȃ�
			char buf_attack[10], buf_sustain[10], buf_release[10];
			if(tone_map[tone_num].brr_fname.substr(0,8)=="FF4inst:" && tone_map[tone_num].attack==0xFF){
				sprintf(buf_attack, "");
				sprintf(buf_sustain, "");
				sprintf(buf_release, "");
			}
			else{
				sprintf(buf_attack, "DC %02X", tone_map[tone_num].attack);
				sprintf(buf_sustain, "DD %02X", tone_map[tone_num].sustain);
				sprintf(buf_release, "DE %02X", tone_map[tone_num].release);
			}
			sprintf(buf, "DB %02X %s %s %s ",
				(tone_map[tone_num].brr_fname.substr(0,9)=="FF4inst:s")
					? tone_map[tone_num].inst_id : (0x40 + tone_map[tone_num].brr_id),
				buf_attack, buf_sustain, buf_release
				);
			str.replace(p, ep-p, buf);
			p--;
			continue;
		}
		// ���[�v�̏���
		if(str[p]==']'){ // ���[�v�̌��
			int sp = skip_space(str, p+1);
			int ep = term_end(str, sp);
			int loop_count = atoi(str.substr(sp, ep-sp).c_str());
			str.replace(p, ep-p, "F0 ");
			// ���[�v�̐擪 [ ��������
			int jump_dest = 0;
			int lp = p;
			for(lp=sp; lp>=0; lp--){
				if(str[lp]=='[') break;
				// ���[�v����F9�W�����v����
				if(str[lp]=='|'){
					str.insert(p+3, "jump_dest ");
					sprintf(buf, "F5 %02X jump_src ", (uint8)loop_count);
					str.replace(lp, 1, buf);
					// "F1 " + "jump_dest " + "F9 XX jump_src "
					jump_dest = 3 + 10 + 15;
				}
			}
			if(lp==-1){
				printf("Error line %d : ] �ɑΉ����� [ ������܂���.\n", line);
				return -1;
			}
			// "[" �� "E0 02 "
			sprintf(buf, "E0 %02X ", (uint8)(loop_count-1));
			str.replace(lp, 1, buf);
			p += (6-1) + (jump_dest-1);
			continue;
		}
		// 10�i������16�i���ɕϊ�
		if(str[p]=='d'){
			int sp = p + 1;
			int ep = term_end(str, sp);
			int d = atoi(str.substr(sp, ep-sp).c_str());
			sprintf(buf, "%02X", (uint8)d);
			str.replace(p, ep-sp+1, buf);
			p--;
			continue;
		}
		// �I�N�^�[�u����
		if(str[p]=='>'){
			sprintf(buf, "E1");
			str.replace(p, 1, buf);
			p++;
			continue;
		}
		if(str[p]=='<'){
			sprintf(buf, "E2");
			str.replace(p, 1, buf);
			p++;
			continue;
		}
		// �R���o�[�g�I��
		if(str[p]=='!'){
			str.erase(p);
			break;
		}
	}

	// �Ō��#track_end_mark��t��
	str.insert(p, "#9", 2);

//FILE *fp=fopen("sample_debug.txt","w");fprintf(fp,str.c_str());fclose(fp);

	return 0;
}

int get_sequence(string &str, SPC &spc)
{
	line = 1;

	const int SEQ_SIZE_MAX = 8*1024;
	uint8 seq[SEQ_SIZE_MAX];
	int seq_size = 0;

	int track_num = 0;
	int seq_id = -1;

	int p;
	for(p=0; str[p]!='\0'; p++){
		if(seq_size>=SEQ_SIZE_MAX){
			printf("Error : �V�[�P���X�T�C�Y�� %d Byte �𒴂��Ă��܂��܂���.\n", SEQ_SIZE_MAX);
			return -1;
		}
		if(str[p]=='\n') line++;
		if(str[p]=='#'){
			//printf("t%d\n", track_num);
			// �g���b�N�̏I��
			if(track_num!=0){
				// �g���b�N���[�v������Ȃ�
				if(spc.track_loop[seq_id]!=0xFFFF){
				//	printf("track%d_loop %d\n", track_id, spc.track_loop[track_id]);
					seq[seq_size++] = 0xF4; // ���[�v�R�}���h
					// ���Βl�����Ă���
					seq[seq_size++] = (uint8)spc.track_loop[seq_id];
					seq[seq_size++] = spc.track_loop[seq_id] >> 8;
				//	printf("t%d seq_size %d\n", track_num, seq_size); getchar();
				}
				// 20191230 F2�ŏI����ĂȂ��Ȃ�F2��u��
				else if(seq[seq_size-1]!=0xF1){
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

			// �g���b�N�̊J�n
			seq_id = track_num - 1;
			if(spc.seq_size[seq_id]!=0){
				printf("Error line %d : #track �g���b�N�i���o�[���d�����Ă��܂�.\n", line);
				return -1;
			}
			seq_size = 0;
			p++;
			continue;
		}
		if(track_num==0) continue;
		// �g���b�N���[�v�̌��o
		if(str[p]=='L'){
			if(spc.track_loop[seq_id]!=0xFFFF){ // ���ł� L ��������
				printf("Error line %d : L �̑��d�g�p�ł�.\n", line);
				return -1;
			}
		//	printf("t%d track_loop %d\n", track_num, seq_size);getchar();
			spc.track_loop[seq_id] = seq_size;
			continue;
		}
		// F5�R�}���h�̏���
		// F5 01 jump_src  XX XX XX F0 jump_dest 
		if(seq_size>1 && seq[seq_size-2]==0xF5 && str.substr(p, 8)=="jump_src"){
			int jp = 2; // F9����̑��΃A�h���X�A2�ō���
			int lp;
			for(lp=p+8; str[lp]!='#'; lp++){
				if(str.substr(lp, 9)=="jump_dest"){
					str.erase(lp, 9); // dest�폜
					// �W�����v�摊�Βl�����Ă���(�K�{)
					char buf[10];
					sprintf(buf, "%02X %02X ", (uint8)jp, (uint8)(jp>>8));
					str.replace(p, 8, buf); // src�u������
					spc.break_point[seq_id].push_back(seq_size); // �W�����v��A�h���X��u���ꏊ
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
		if(is_cmd(str[p])){
			seq[seq_size++] = get_hex(str, p);
		//	printf("0x%02X ",get_hex(str, p));getchar();
			p++;
			continue;
		}
	}

	// track���Ȃ��Ȃ�V�[�P���X�I����u���Ă���
	int t;
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

#include<time.h>

int make_spc(SPC &spc, FF4_AkaoSoundDriver &asd, const char *spc_fname)
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
	// �o�C�i���t�H�[�}�b�g
	int i;
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

	// SPC�쐬�N����(16�i)
	time_t timer = time(NULL);
	struct tm *local;
	local = localtime(&timer); // �n�����ɕϊ�
	header[0x9E] = local->tm_mday; // ��
	header[0x9F] = local->tm_mon+1; // ��
	*(uint16*)(header+0xA0) = local->tm_year+1900; // �N

	// �Đ�����(3byte)
	*(uint32*)(header+0xA9) = spc.play_time;
	// �t�F�[�h�A�E�g����(4byte)
	*(uint32*)(header+0xAC) = spc.fade_time;


	// �T�E���h������
	uint8 *ram = new uint8[0x10000];
	memset(ram, 0x00, 0x10000);
	ram[0x00F1] = 0x03; // ctrl
	ram[0x00FA] = 0x24; // target0
	ram[0x00FB] = 0x01; // target1
	ram[0x01CD] = 0x99; // �X�^�b�N
	ram[0x01CE] = 0x08; // �X�^�b�N
	ram[0x01CF] = 0x08; // �X�^�b�N


	// DSP������
	uint8 dsp_reg[128];
	memset(dsp_reg, 0x00, 128);
	dsp_reg[0x0C] = 0x40; // MVOL_L
	dsp_reg[0x1C] = spc.f_surround ? 0xC0 : 0x40; // MVOL_R
	dsp_reg[0x5D] = 0x1E; // DIR
	dsp_reg[0x6C] = 0x20; // FLG
	// �G�R�[�o�b�t�@�̈�ݒ�
	uint16 echobuf_start_adrs = 0xF800 - (spc.echo_depth << 11);
//	dsp_reg[0x6D] = echobuf_start_adrs >> 8; // ESA
//	dsp_reg[0x7D] = spc.echo_depth; // EDL


	// �x�[�X�A�h���X
	*(uint16*)(ram+0x2000) = 0x2010;
	// �h���C�o
	memcpy(ram+0x0800, asd.driver, asd.driver_size);
	// �A�^�b�N�e�[�u���A�h���X
	memcpy(ram+0x1D00, asd.attack_table_start, asd.attack_table_start_size);
	// �A�^�b�N�e�[�u��
	memcpy(ram+0xF900, asd.attack_table, asd.attack_table_size);
	// �풓�g�`�����␳
	memcpy(ram+0xFF00, asd.sbrr_tune, 8);
	// ���ʉ��V�[�P���X��
//	memcpy(ram+0xXX00, asd.eseq, asd.eseq_size); // �g�p���Ȃ�

	// �����␳���ߍ���
	int i;
	for(i=0; i<spc.brr_map.size(); i++){
		if(spc.brr_map[i].brr_fname.substr(0, 8)=="FF4inst:"){
			int sp = 8;
			int ep = term_end(spc.brr_map[i].brr_fname, sp);
			int inst_id = atoi(spc.brr_map[i].brr_fname.substr(sp, ep-sp).c_str());
			ram[0xFF40+i] = asd.brr_tune[inst_id];
		}
		else{ // ����BRR
			ram[0xFF40+i] = 0x00; // �����␳�͂ǂ����悤���H
		}
	}

	// 0x2010����V�[�P���X�f�[�^
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

	// �V�[�P���X�f�[�^�𖄂ߍ���
	for(i=0; i<8; i++){
		memcpy(ram + seq_adrs[i], spc.seq[i], spc.seq_size[i]);
	}

	// BRR�ʒuauto
	uint16 brr_offset = spc.brr_offset;
	if(spc.brr_offset==0xFFFF){
		brr_offset = seq_adrs_end;
	}
	seq_adrs_end--;
	printf("SEQ end address 0x%04X\n", seq_adrs_end); //getchar();
	if(seq_adrs_end >= spc.brr_offset){
		printf("Error : #brr_offset 0x%04X ���V�[�P���X�Əd�Ȃ��Ă��܂�.\n", spc.brr_offset);
		delete[] ram;
		return -1;
	}

	// �V�[�P���X�A�h���X�̖��ߍ���
	for(i=0; i<8; i++){
		*(uint16*)(ram+0x2000+i*2) = seq_adrs[i];
	}

	// �g���b�N���[�v�A�h���X���ߍ���
	// F4 XX XX
	{
	int t, i;
	for(t=0; t<8; t++){
	//	printf("seq%d_size %d\n", t, spc.seq_size[t]);
		if(spc.track_loop[t]!=0xFFFF){
			for(i=spc.seq_size[t]-1; i>=0; i--){
				// �g���b�N�Ō��F4�ɑ΂��Ă̂݃��[�v�A�h���X�𒲐�
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
	// ���[�v�u���C�N�W�����v���ߍ���
	// F5 NN XX XX
	for(t=0; t<8; t++){
	//	printf("seq%d_size %d\n", t, spc.seq_size[t]);
		for(i=0; i<spc.break_point[t].size(); i++){
		//	printf("jump_rel %d  %d + %d\n", i, spc.break_point[t][i], *(uint16*)(spc.seq[t]+spc.break_point[t][i]));
			uint16 jump_adrs = seq_adrs[t] + spc.break_point[t][i] + *(uint16*)(spc.seq[t]+spc.break_point[t][i]);
		//	printf("jump_adrs 0x%04X\n", jump_adrs);
			*(uint16*)(ram+seq_adrs[t]+spc.break_point[t][i]) = jump_adrs;
		}
	}
	}

	// BRR���ߍ���
	// ���łɖ��ߍ���BRR�͎g���܂킷
	map<string, pair<uint16, uint16> > brr_put_map;
	uint32 adrs_index = 0;
	for(i=0; i<spc.brr_map.size(); i++){
		string brr_fname = spc.brr_map[i].brr_fname;

		uint32 start_adrs, loop_adrs;
		if(brr_put_map.find(brr_fname)==brr_put_map.end()){ // �u���ĂȂ�brr�Ȃ�

			int brr_size;
			uint8 *brr_data;
			if(brr_fname.substr(0, 8)=="FF4inst:"){ // FF4�̔g�`
				int sp = 8;
				int ep = term_end(brr_fname, sp);
				int inst_id = atoi(brr_fname.substr(sp, ep-sp).c_str());
				brr_size = asd.brr_size[inst_id] + 2; // �擪2�o�C�g���[�v�ǉ�
				brr_data = new uint8[brr_size];
				memcpy(brr_data+2, asd.brr[inst_id], asd.brr_size[inst_id]);
				*(uint16*)brr_data = asd.brr_loop[inst_id];
			//	printf("loop 0x%04X\n",asd.brr_loop[inst_id]);
			}
			else{ // BRR�t�@�C���w��
				FILE *brrfp = fopen(brr_fname.c_str(), "rb");
				if(brrfp==NULL){
					printf("Error : BRR�t�@�C�� %s ���J���܂���.\n", brr_fname.c_str());
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
				printf("Error : %s BRR�G���A��0xF900�𒴂��܂���.\n", brr_fname.c_str());
				delete[] brr_data;
				delete[] ram;
				return -1;
			}
			memcpy(ram+start_adrs, brr_data+2, brr_size-2);
			delete[] brr_data;

			if(start_adrs >= 0x0F900){
				printf("BRR start address 0x%X\n", start_adrs);
				printf("Error : %s BRR�X�^�[�g�A�h���X��0xF900�𒴂��܂���.\n", brr_fname.c_str());
				delete[] ram;
				return -1;
			}
			if(loop_adrs >= 0x0F900){
				printf("BRR loop address 0x%X\n", loop_adrs);
				printf("Error : %s BRR���[�v�A�h���X��0xF900�𒴂��܂���.\n", brr_fname.c_str());
				delete[] ram;
				return -1;
			}

			brr_put_map[brr_fname] = make_pair<uint16, uint16>(start_adrs, loop_adrs);
			adrs_index += brr_size -2;
		}
		else{ // ���łɔz�u����brr�Ȃ�A�h���X�𗬗p���邾��
			start_adrs = brr_put_map[brr_fname].first;
			loop_adrs = brr_put_map[brr_fname].second;
		}

		*(uint16*)(ram+0x1F00+i*4) = (uint16)start_adrs;
		*(uint16*)(ram+0x1F02+i*4) = (uint16)loop_adrs;
	}

	if(spc.brr_offset==0xFFFF){ // auto
		brr_offset += (uint16)adrs_index;
	}
	else{
		brr_offset = 0xCA70;
	}
	// �풓�g�`BRR
//	memcpy(ram+0xCA70, asd.sbrr, asd.sbrr_size);
	memcpy(ram+brr_offset, asd.sbrr, asd.sbrr_size);
	// �풓�g�`�A�h���X�ύX
	uint16 brr_adrs_sa = 0xCA70 - brr_offset;
	for(i=0; i<7; i++){
		asd.sbrr_start[  i*2] -= brr_adrs_sa;
		asd.sbrr_start[1+i*2] -= brr_adrs_sa;
	}
	// �풓�g�`BRR�A�h���X
	memcpy(ram+0x1E00, asd.sbrr_start, 32);

	uint32 brr_adrs_end = (uint32)brr_offset + asd.sbrr_size -1;
	printf("BRR end address 0x%04X\n", brr_adrs_end); //getchar();
	printf("EchoBuf start   0x%04X\n", echobuf_start_adrs);//getchar();
	if(spc.f_brr_echo_overcheck){
		if((uint16)brr_adrs_end >= echobuf_start_adrs){
			printf("Error : BRR�f�[�^���G�R�[�o�b�t�@�̈�Əd�Ȃ��Ă��܂�.\n");
			delete[] ram;
			return -1;
		}
	}


	// SPC�o��
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
	printf("%s �𐶐����܂���.\n", spc_fname);
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

int main(int argc, char *argv[])
{
	printf("spcmake_byFF4 ver.20200112\n");

#ifdef _DEBUG
	argc = 3;
	argv[1] = "sample.txt";
	argv[2] = "sample.spc";
#endif

	if(argc!=3){
		printf("useage : spcmake_byFF4.exe input.txt output.spc\n");
		getchar();
		return -1;
	}

	FF4_AkaoSoundDriver asd;

	if(get_akao("FinalFantasy4.rom", asd)){
		return -1;
	}

	string str;

	// �V�[�P���X�t�@�C���ǂݍ���
	FILE *fp = fopen(argv[1], "r");
	if(fp==NULL){
		printf("MML�t�@�C�� %s ���J���܂���.\n", argv[1]);
		return -1;
	}
	char buf[1024];
	while(fgets(buf, 1023, fp)){
		str += buf;
	}
	fclose(fp);

	SPC spc;

	if(formatter(str, asd, spc)){
#ifdef _DEBUG
	getchar();
#endif
		return -1;
	}

//fp=fopen("out.txt","w");fprintf(fp,str.c_str());fclose(fp);

	printf("songname[%s]\n", spc.songname.c_str());
	printf("dumper[%s]\n", spc.dumper.c_str());

	if(get_sequence(str, spc)){
		return -1;
	}

	if(make_spc(spc, asd, argv[2])){
		return -1;
	}

//	getchar();

	return 0;
}
