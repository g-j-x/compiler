#include <iostream>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <queue>
#include <stack>
#include <algorithm>
#include <fstream>
#include <sstream>
using namespace std;
/* TOKEN集合 */
typedef enum {
    /*状态 */
    ENDL,ERROR,
    /*关键字 */
    IF,ELSE,INT,CHAR,FLOAT,RETURN,VOID,WHILE,FOR,AND,OR,NOT,PRINT,
    /*标识符 */
    ID,NUMA,NUMB,NUMC,NUMD,NUME,NUMF,
    /*运算符*/
    /*+ - * / < <= > >= == != = ; , ( ) [ ] { } # :*/
    ADD,SUB,MUL,DIV,LT,LE,GT,GE,EE,EQ,NE,SEMI,COMMA,LPAR,RPAR,LBRA,RBRA,LCUR,RCUR,TAG,COLON
} TokenType;
/* DFA状态集合*/
typedef enum {          //A.BDeCFE    对复数的判定从A开始 
    START,SID,SNUMA,SNUMB,SNUMC,SNUMD,SNUME,SNUMF,SNUMG,
    SGT,SLT,SEQ,SNE,SEE,DONE
} StateType;
map<string,TokenType> keyWords;
map<string,StateType> keyState;
map<string,string> category;
struct Ans {
	int line;			//词法所在行
	int column;			//词法所在列
	string classify;	//词法token类型
	string value;		//词法内容
	string error;		//错误类型
	Ans(int l,int c,string cl,string v,string e) {  //构造函数
		line=l;
		column=c;
		classify=cl;
		value=v;
		error=e;
	}
};
struct Node {
	char pre,next;   //当前状态，下一状态，转换条件
	string data;
	Node(char p,string d,char n) {  //构造函数
		pre=p;
		data=d;
		next=n;
	}
};
struct FA {
	int edgeNum;			// 边数统计
	int nodeNum;			// 节点统计
	string state;		//有穷状态集
	string alphabet;		//有穷字母表
	vector<Node>tranSet;	//转换函数矩阵，其大小为tranSet[nodeNum][nodeNum]
	string startNode;		//非空初态集
	vector<char> endNode;	//终态集
} DFA,NFA;
//正规文法结构体
struct EX {
	set<char> VN;			//非终结符
	string VT;				//终结符
	string S;				//开始符
	vector<Node>P;			//产生式
} express;
void preInit();
void init();
void inputEX(string rule);
set<char> Closure(FA nfa, char st);
set<char> unionSet(set<char>s1,set<char>s2);
set<char> e_move(FA nfa,char st, char ch);
bool equalSet(set<char> s1, set<char> s2);
void createNFA(string express);
FA createDFA(FA nfa);
bool isLetter(char ch);
bool isDigit(char ch);
map<char,char> getTrans(char nowNode);
Ans runDFA(string str,int pos,int line);
void preInit() {
	keyWords["if"]=IF;
	keyWords["else"]=ELSE;
	keyWords["int"]=INT;
	keyWords["char"]=CHAR;
	keyWords["float"]=FLOAT;
	keyWords["return"]=RETURN;
	keyWords["void"]=VOID;
	keyWords["while"]=WHILE;
	keyWords["for"]=FOR;
	keyWords["and"]=AND;
	keyWords["or"]=OR;
	keyWords["not"]=NOT;
	keyWords["print"]=PRINT;
	keyState["START"]=START;
	keyState["ID"]=SID;
	keyState["NUMA"]=SNUMA;
	keyState["NUMB"]=SNUMB;
	keyState["NUMC"]=SNUMC;
	keyState["NUMD"]=SNUMD;
	keyState["NUME"]=SNUME;
	keyState["NUMF"]=SNUMF;
	keyState["NUMG"]=SNUMG;
	keyState["GT"]=SGT;
	keyState["LT"]=SLT;
	keyState["EQ"]=SEQ;
	keyState["NE"]=SNE;
	keyState["EE"]=SEE;
	keyState["DONE"]=DONE;
}
void init() {
	fstream fp("./3.txt",ios::in);
	if(!fp) {
		cout<<"文件打开失败"<<endl;
		exit(-1);
	}
	string rule;
	express.S=keyWords["START"]+'A';
	while(!fp.eof()) {
		fp>>rule;
		inputEX(rule);
	}
}
void inputEX(string rule) {       //存储三型文法进入数据结构
	int pre=rule.find('-');
	int last=rule.find('>')+1;
	string vn1=rule.substr(0,pre);//非终结符 左边的
	string vt=rule.substr(last,1);  //终结符 紧邻的
	string vn2=rule.substr(last+1,rule.length()-last-1); //非终结符 终结符右边的
	express.VN.insert(keyState[vn1]+'A');
	express.VN.insert(keyState[vn2]+'A');
	if(express.VT.find(vt)==string::npos&&(vt[0]!='$')) { //找不到这个非终结符并且该非终结符不为$
		express.VT+=vt;
	}
	express.P.push_back(Node((keyState[vn1]+'A'),vt,(keyState[vn2]+'A')));
}
// 返回一个节点的ε闭包
set<char> Closure(FA nfa, char st) {
	int pos;
	set<char> result;
	stack<char> s;
	s.push(st);
	while (!s.empty()) {
		char ch =s.top();
		s.pop();
		result.insert(ch);  //闭包包含自己

		vector<int> pos;
		for(int i=0; i<nfa.tranSet.size(); i++) { //通过pos定位第i条转换边
			if(nfa.tranSet[i].pre==ch) {  //ch为起点
				pos.push_back(i);         //如果以ch为起点有转换关系就记录
			}
		}
		for(int i=0; i<pos.size(); i++) {
			if(nfa.tranSet[pos[i]].data[0]=='$') {  //如果这条转换关系的转换条件是空$
				s.push(nfa.tranSet[pos[i]].next);//通过e能到达
			}
		}
	}
	return result;
}
//合并两个集合
set<char> unionSet(set<char>s1,set<char>s2) {
	s1.insert(s2.begin(),s2.end());
	return s1;
}
//返回一个节点st经过转换字符ch转换得到的结果（ε闭包结果）
set<char> e_moveSet(FA nfa,char st, char ch) {
	set<char> result;
	vector<int> pos;
	for(int i=0; i<nfa.tranSet.size(); i++) {
		if(nfa.tranSet[i].pre==st) {
			pos.push_back(i);
		}
	}
	for(int i=0; i<pos.size(); i++) { //move(T,a)
		if(nfa.tranSet[pos[i]].data[0]==ch) {
			result.insert(nfa.tranSet[pos[i]].next);
		}
	}
	for(set<char>::iterator it=result.begin(); it!=result.end(); it++) {
		// 将其ε-闭包加入结果
		result=unionSet(result,Closure(nfa,*it));  //使用set 不用担心元素重复 
	}
	return result;
}
// 判断两个集合是否相同
bool equalSet(set<char> s1, set<char> s2) {
	// 大小不一样的集合必然不同
	if (s1.size()!=s2.size()) {
		return false;
	}
	set<char> temp=unionSet(s1,s2);

	if (temp.size()!=s1.size()) {
		return false;
	} else {
		return true;
	}
}
void createNFA(EX express) {
	NFA.nodeNum=express.VN.size(); //有多少个非终结符就有多少点
	NFA.edgeNum=express.P.size(); //由于是3型文法 一个产生式对应一条边
	NFA.alphabet=express.VT;//字母表
	NFA.startNode=express.S;
	NFA.tranSet=express.P;//产生式
	for(set<char >::iterator it=express.VN.begin(); it!=express.VN.end(); it++) {
		NFA.state+=*(it);       //第一个非终结符就是初态
	}//15状态（A~O）
	int flag;
	//查找终止节点
	for(int i=0; i<NFA.state.size(); i++) {
		flag=0;  //每次进来都置0 对每个状态都判断是不是终结态
		for(int j=0; j<NFA.tranSet.size(); j++) {
			if(NFA.tranSet[j].pre==NFA.state[i]) {
				flag=1;  //找到匹配的产生式  该状态不会是终结状态
				break;
			}
		}
		if(!flag) {             //flag未被置1，没有对应的产生式，即为终结态
			NFA.endNode.push_back(NFA.state[i]);
			flag=0;
		}
	}
	//写文件
	ofstream out("./NFA.txt");
	if(out.fail()) {
		cout<<"输出流错误！";
	}
	for(int i=0; i<NFA.tranSet.size(); i++) {
		out<<NFA.tranSet[i].pre<<"##"<<NFA.tranSet[i].data<<"##"<<NFA.tranSet[i].next<<endl;
	}
	out.close();

}
FA createDFA(FA nfa) {
	FA dfa;
	vector<set<char> > Q;//所有DFA节点集合
	queue<set<char> > s;//根据子集变化情况，控制循环    s会和遍历所有DFA的节点直到节点数不再增多 

	s.push(Closure(nfa,nfa.startNode[0]));//加入初始状态的闭包 
	Q.push_back(Closure(nfa,nfa.startNode[0]));

	queue<set<char> > transEdge;//保存转换结果，和Q一一对应

	while(!s.empty()) {
		set<char> top=s.front();//得到队首元素一个set
		s.pop();		//队首元素出队

		//对每一个转换的字符，求转换结果
		for(int i=0; i<nfa.alphabet.size(); i++) {
			set<char>temp;
			int flag=0;

			//求move(top,i)
			for(set<char>::iterator it=top.begin(); it!=top.end(); it++) {
				temp=unionSet(temp,e_moveSet(nfa,*it,nfa.alphabet[i]));
			}       //temp为top中所有元素move后经过eclosure后组成的一个set   DFA的一个节点 

			// 如果当前节点经过转换字符转换到空集，则加入一个表示空集的符号
			if(temp.size()==0) {
				temp.insert('$');
			} 
			else { //遍历Q,将未标记子集加入栈中
				for(vector<set<char> >::iterator Qit=Q.begin(); Qit!=Q.end(); Qit++) {
					if(equalSet(*Qit,temp)) {
						flag=1;
					}
				}
				if(flag==0) {     //这是个新的节点
					s.push(temp);
					Q.push_back(temp);
				}
			}
			transEdge.push(temp); //转换结果入队列  不管是不是新节点都要加入到transEdge中 这是转换的结果 
		}
	}
	//构造新的DFA
	int i,j;
	dfa.nodeNum=Q.size();
	dfa.alphabet=nfa.alphabet;//构造字母表

	//构造节点（状态）
	for(i=0; i<dfa.nodeNum; i++) {
		dfa.state+=i+'A';  //Q中第一个就是初态
	}

	//对每一个转换的字符，构造转换关系
	i=0;
	while(!transEdge.empty()) {    //transEge里面有每个状态Q通过所有alphabet走到的状态Q 
		for(int p=0; p<dfa.alphabet.size(); p++) {
			set<char>temp=transEdge.front();//temp为transEgde队首set元素  一个eclosure(move(x,x))  一个DFA节点 
			transEdge.pop();
			if(temp.size()==1&&*temp.begin()=='$') {  //如果这个节点包含的是空$
				continue;
			} else {
				for(j=0; j<Q.size(); j++) { //找出位置j,转移结果的标号   有多个temp是Q通过alphabet转换得来的
					if(equalSet(temp,Q[j])) {  //找到当前的状态Q[i]通过alphbet转换到了那个状态Q[j] 
						break;
					}
				}
				string str;  //存储node.data 
				str.push_back(dfa.alphabet[p]);
				dfa.tranSet.push_back(Node(i+'A',str,j+'A'));
			}
		}
		i++;  //遍历完一次alphbet就代表遍历完了从第一个状态通过alphbet到达得所有状态集合 开始进入下一个状态
	}

	dfa.edgeNum=dfa.tranSet.size(); //边数就是转换多少个Q到transEdge
	//构造初态
	dfa.startNode=dfa.state[0];  //245行 Q中第一个就是初态

	//查找终态
	int flag;
	//查找终止节点
	//新状态T含有原来终态，则新状态为终态
	for(i=0; i<Q.size(); i++) {
		for(int j=0; j<nfa.endNode.size(); j++) {
			set<char>::iterator it=Q[i].find(nfa.endNode[j]);  //dfa的所有状态中即Q中包含由原nfa中的终结态就为终结态Q[i]为一个状态
			if(it!=Q[i].end()) {     //如果find没找到了，就代表这是状态Q包含nfa中终结符的状态 
				dfa.endNode.push_back(dfa.state[i]); //Q加入终态 
			}
		}
	}
	//实例化终结符
	for(int i=0; i<dfa.tranSet.size(); i++) {
		if(dfa.tranSet[i].data[0]=='l') {
			dfa.tranSet[i].data.clear();          //转换的那条边如果是l代表了字母
			dfa.tranSet[i].data.insert(0,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
		} else if(dfa.tranSet[i].data[0]=='d') {
			dfa.tranSet[i].data.clear();     //转换的那条边如果是d 代表了数字
			dfa.tranSet[i].data.insert(0,"0123456789");
		}
	}
	//写文件
	ofstream out("./DFA.txt");
	if(out.fail()) {
		cout<<"输出流错误！";
	}
	for(int i=0; i<dfa.tranSet.size(); i++) {
		out<<dfa.tranSet[i].pre<<"##"<<dfa.tranSet[i].data<<"##"<<dfa.tranSet[i].next<<endl;
	}
	out.close();

	return dfa;
}
//判断是否是字母
bool isLetter(char ch) {
	if((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')) {
		return true;
	}
	return false;
}
//判断是否是数字
bool isDigit(char ch) {
	if(ch>='0'&&ch<='9') {
		return true;
	}
	return false;
}
map<char,char> getTrans(char nowNode) {
	map<char,char>transfer;
	for(int i=0; i<DFA.tranSet.size(); i++) {
		for(int j=0; j<DFA.tranSet[i].data.size(); j++) {
			if(DFA.tranSet[i].pre==nowNode) {
				transfer[DFA.tranSet[i].data[j]]=DFA.tranSet[i].next;		//建立转换关系
			}
		}
	}
	return transfer;
}
string symbol="+-*/<>=!;,()[]{}:";
string limiter=";,()[]{}:";
string operators="+-*/<>=!";
Ans runDFA(string str,int pos,int line) {    //
	string tokenType;  //token类型 
	string endState;   //DFA终结态集合 
	for(int i=0; i<DFA.endNode.size(); i++) {
		endState.push_back(DFA.endNode[i]);
	}
	//判断标识符
	if(isLetter(str[pos])) { //第一个字符就能表明它是不是标识符
		int finalPos=pos;
		string finalStr;
		//预读取一个字符串
		while(finalPos<str.length()&&symbol.find(str[finalPos])==string::npos&&str[finalPos]!=' ') { //找不到symbol中的字符并且忽略空格   标识符不能含有symbol和空格
			finalStr+=str[finalPos];
			finalPos++;
		}
		int curPos=0;
		string token;
		char nowNode=START+'A';//开始
		//提取DFA中信息
		map<char,char>transfer=getTrans(nowNode);//得到从A  即开始符出去能得到的转换关系
		while(curPos<finalStr.length()&&(transfer.find(finalStr[curPos])!=transfer.end())) {
			token+=finalStr[curPos];
			nowNode=transfer[finalStr[curPos]];
			transfer.clear();
			transfer=getTrans(nowNode);//得到转换之后  从这个状态能得到的转换关系
			curPos++;
		}
		if(curPos>=finalStr.length()&&endState.find(nowNode)!=string::npos) { //如果finalStr中的每个字符被完全识别并且自动机的nowNode处于终结态 表示成功识别
			tokenType.clear();
			if(keyWords.find(token)!=keyWords.end()) { //如果在keywords映射中找到这个token能够映射到一个keyword
				tokenType.insert(0,"keyword");
			} else {            //否则就是标识符
				tokenType.insert(0,"identifier");
			}
			return Ans(line,finalPos-1,tokenType,token,"OK");
		} else {
			return Ans(line,finalPos-1,"","","标识符非法！");
		}
	} else if(isDigit(str[pos])) { //判断数字
		//判断
		int finalPos=pos;
		string finalStr;
		//预读取一个字符串
		while(finalPos<str.length()&&(symbol.find(str[finalPos])==string::npos||str[finalPos]=='+'||str[finalPos]=='-')&&str[finalPos]!=' ') { //   + - 能够满足是因为有复数的存在1+i 1-i
			finalStr+=str[finalPos];
			finalPos++;
		}
		int curPos=0;
		string token;
		char nowNode=START+'A';
		//提取DFA中信息
		map<char,char>transfer=getTrans(nowNode);  //transfer是对一个节点来说 每条边到其它节点的映射  S通过a到达B第一个char为a,第二个char为B
		while(curPos<finalStr.length()&&(transfer.find(finalStr[curPos])!=transfer.end())) {
			token+=finalStr[curPos];
			nowNode=transfer[finalStr[curPos]];  //渡过一条边 到下一个节点
			transfer.clear();
			transfer=getTrans(nowNode);//得到转换之后  从这个状态能得到的转换关系
			curPos++;
		}
		if(curPos==finalStr.length()&&endState.find(nowNode)!=string::npos) {
			tokenType.clear();
			tokenType.insert(0,"number");
			return Ans(line,finalPos-1,tokenType,token,"OK");
		}
		return Ans(line,finalPos-1,"","","常量非法！");
	}
	else {  //既不是字符也不是数字 
		int curPos=pos;
		string token;
		tokenType.clear();
		tokenType.insert(0,"limiter");  //首先判定是限定符 
		char nowNode=START+'A';
		//提取DFA中信息
		map<char,char>transfer=getTrans(nowNode);

		while(curPos<str.length()&&(limiter.find(str[curPos])!=string::npos)&&(transfer.find(str[curPos])!=transfer.end())) {
			token+=str[curPos];
			nowNode=transfer[str[curPos]];
			transfer.clear();
			transfer=getTrans(nowNode);
			curPos++;
		}
		if(endState.find(nowNode)!=string::npos) {   //如果到了终态就表明是限定符，如果不是就说明是操作符 
			return Ans(line,curPos-1,tokenType,token,"OK");
		}
		curPos=pos;
		token.clear();
		tokenType.clear();
		tokenType.insert(0,"operator");
		while(curPos<str.length()&&(operators.find(str[curPos])!=string::npos)&&(transfer.find(str[curPos])!=transfer.end())) {
			token+=str[curPos];
			nowNode=transfer[str[curPos]];
			transfer.clear();
			transfer=getTrans(nowNode);
			curPos++;
		}
		if(endState.find(nowNode)!=string::npos) {
			return Ans(line,curPos-1,tokenType,token,"OK");
		}
		return Ans(line,curPos-1,"","","其他错误");
	}
}
int main() {
	preInit();//初始化、状态赋值
	init();//初始化文法表达式
	createNFA(express);
	DFA=createDFA(NFA);
	fstream fp("./test1.txt",ios::in);
	if(!fp) {
		cout<<"输入流出错！"<<endl;
		exit(-1);
	}
	string str;
	int lineNum=0;
	bool lexError=false;
	vector<Ans> tokenTable;
	while(!fp.eof()) {
		getline(fp,str);//读取一行
		int pos=0;
		lineNum++;
		while(pos<str.length()) {
			while(pos<str.length()&&(str[pos]=='\t'||str[pos]=='\n'||str[pos]==' '||str[pos]=='\r')) {
				pos++;
			}//通过pos定义非\t \n  \r 开始的位置
			if(pos<str.length()) {
				Ans temp=runDFA(str,pos,lineNum);
				pos=temp.column;
				if(temp.classify.length()==0) {
					cout<<"error at line "<<temp.line<<" column "<<temp.column<<" : "<<temp.error<<endl;
					lexError=true;
					break;
				} else {
					tokenTable.push_back(temp);
					cout<<"("<<temp.line<<","<<temp.classify<<","<<temp.value<<")"<<endl;
				}
				pos++;
			}
		}
	}
	if(!lexError) {
		//写文件
		ofstream out("ans.txt");
		if(out.fail()) {
			cout<<"输出流错误！";
		}
		for(int i=0; i<tokenTable.size(); i++) {
			out<<tokenTable[i].line<<" "<<tokenTable[i].classify<<" "<<tokenTable[i].value<<endl;
		}
		out.close();
		system("pause");
		return 0;
	}
	cout<<"词法分析出错！";
	system("pause");
	return 0;
}

