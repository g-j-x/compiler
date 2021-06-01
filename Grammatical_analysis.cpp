#include <iostream>
#include <cstring>
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
//产生式 
struct Estring {
	string left;	//产生式左部
	vector<string> right;	//产生式右部
	Estring(string l, vector<string> r) {
		left = l;
		right = r;
	}
};
//文法
struct Grammar {
	vector<Estring> expressions;		//产生式集合
	string S;							//开始字符
	set<string> Vn;						//非终结符集
	set<string> Vt;						//终结符集
	map<string, set<string> > firstSet;		//First集
	map<string, set<string> > followSet;    //Follow集
	vector<set<string> > selectSet; //select集
	vector<vector<int> > table;  //预测分析表
}G;
int ll1=1;
vector<string> tokens;
void init();
void initVt();
void inputEX(string rule);
void calFisrt();
void calFollow();
void calSelect();
int get_column(string target);
int get_row(string target);
void create_table();
void print_table();
vector<string> split(const string& str, const string& delim);
void deal_tokens();
//转换函数 
void init() {
	fstream fp("./2.txt", ios::in);
	if (!fp) {
		cout << "文件打开失败" << endl;
		exit(-1);
	}
	string rule;
	G.S = "S";
	initVt();
	while (!fp.eof()) {
		getline(fp, rule);
		inputEX(rule);
	}
}
void initVt() {
	G.Vt.insert("+");//operators
	G.Vt.insert("-");
	G.Vt.insert("*");
	G.Vt.insert("/");
	G.Vt.insert(">");
	G.Vt.insert(">=");
	G.Vt.insert("<");
	G.Vt.insert("<=");
	G.Vt.insert("=");
	G.Vt.insert("==");
	G.Vt.insert("!=");
	G.Vt.insert("$");//limiter
	G.Vt.insert(";");
	G.Vt.insert(",");
	G.Vt.insert("(");
	G.Vt.insert(")");
	G.Vt.insert("{");
	G.Vt.insert("}");
	G.Vt.insert("char");//type
	G.Vt.insert("int");
	G.Vt.insert("void");
	G.Vt.insert("float");
	G.Vt.insert("number");
	G.Vt.insert("identifier");
	G.Vt.insert("or");
	G.Vt.insert("and");
	G.Vt.insert("not");
	G.Vt.insert("if");
	G.Vt.insert("else");
	G.Vt.insert("while");
	G.Vt.insert("for");
	G.Vt.insert("return");
	G.Vt.insert("print");
}
void inputEX(string rule) {//生成2型文法 
	int pre = rule.find('-');
	int last = rule.find('>') + 1;
	string left = rule.substr(0, pre);
	G.Vn.insert(left);
	vector<string> right;
	int pos = rule.find('>');
	for (int i = last; i < rule.length(); i++) {
		if (rule[i] == ' ') {
			pos = i;
			string cur = rule.substr(last, i - last);
			right.push_back(cur);
			if (G.Vn.find(cur) == G.Vn.end() && G.Vt.find(cur) == G.Vt.end()) {
				G.Vn.insert(cur);
			}
			last = i + 1;
		}
	}
	string cur = rule.substr(pos + 1, rule.length());
	right.push_back(cur);
	if (cur == "$") {
		right.clear();
	}
	if (G.Vn.find(cur) == G.Vn.end() && G.Vt.find(cur) == G.Vt.end()) {
		G.Vn.insert(cur);
	}

	G.expressions.push_back(Estring(left, right));
}
void calFisrt() {
	set<string> vn;
	int esize, flag = 1;
	for (int i = 0; i < G.expressions.size(); i++) {    //产生式的数量
		vn.insert(G.expressions[i].left);            //vn存放了所有产生式左端的非终结符  使用set使其唯一
	}
	while (flag) {
		flag = 0;
		for (set<string>::iterator it = vn.begin(); it != vn.end(); it++) {    //遍历所有非终结符
			esize = G.firstSet[*it].size();  //当前终结符的first集合大小
			for (int i = 0; i < G.expressions.size(); i++) {   //对每一个非终结符遍历所有产生式
				if (G.expressions[i].left == *it) {// 左端与目标匹配
					if (G.expressions[i].right.size() == 0) {  // right是$时 终结符直接加入
						G.firstSet[*it].insert("$");
					}
					else {// 非终结符则需逐字符求解，直到遇到终结符，或者全部能够推出ε
						for (int j = 0; j < G.expressions[i].right.size(); j++) {
							string str = G.expressions[i].right[j];
							if (G.Vt.find(str) != G.Vt.end()) {  //如果右边的第一个就是非终结符 ，那直接插入这个非终结符 然后跳出循环
								G.firstSet[*it].insert(str);
								break;
							}
							else if (G.Vn.find(str) != G.Vn.end()) { // 如果这个是非终结符
								if (G.firstSet[str].find("$") == G.firstSet[str].end()) {  //检查这个终结符的first集合是否含有空$
									G.firstSet[*it].insert(G.firstSet[str].begin(), G.firstSet[str].end());//如果不含有空 就把该非终结符的first集并入当前所在处理的非终结符的first集
									break;
								}
								else {  //如果含有空，就把该非终结符除了$的所有first集合元素加入*it中的first集合中，这里并不跳出循环
									for (set<string>::iterator init = G.firstSet[str].begin(); init != G.firstSet[str].end(); init++) {
										if (*init != "$") {
											G.firstSet[*it].insert(*init);
										}
									}
								}

							}
						}
					}
				}
			}
			if (esize != G.firstSet[*it].size())
				flag = 1;  //如果有一个非终结符的first集合的大小发生变化， 就重新遍历所有终结符
		}
	}
}
void calFollow() {
	set<string> vn;
	int esize, flag = 1;
	G.followSet[G.S].insert("#");    //初始化开始符的follow集中含有# 
	for (int i = 0; i < G.expressions.size(); i++) {    //产生式的数量
		vn.insert(G.expressions[i].left);            //vn存放了所有产生式左端的非终结符  使用set使其唯一
	}
	while (flag) {
		flag = 0;
		for (int i = 0; i < G.expressions.size(); i++) {
			string left = G.expressions[i].left;
			for (int j = 0; j < G.expressions[i].right.size(); j++) {
				string right = G.expressions[i].right[j]; //产生式右边的j号元素
				if (vn.find(right) != vn.end()) {       //如果产生式右边这个元素是非终结符
					string bright;//准备存储right后面的元素
					esize = G.followSet[right].size();
					if (j < G.expressions[i].right.size() - 1) {	// Follow(rj)+=First(brj)  right元素后面还有元素
						bright = G.expressions[i].right[j + 1];  //存储right后面的元素
						if (G.Vt.find(bright) != G.Vt.end()) { //如果这是个终结符 就直接加入到follow集中
							G.followSet[right].insert(bright);
						}
						else {    //else 这是个非终结符
							int emptyFlag = 0;  //看看bright元素的first集合是否为空
							set<string>::iterator bright_iter;
							for (bright_iter = G.firstSet[bright].begin(); bright_iter != G.firstSet[bright].end(); bright_iter++) {
								if (*bright_iter != "$") //将bright元素的first集合中的非空元素加入right的follow集合中
									G.followSet[right].insert(*bright_iter);
								else
									emptyFlag = 1;//表示bright元素的first集合中含有空元素$
							}
							if (emptyFlag == 1) {		// A->aBCD，C若能推出ε，Follow(B)+=Follow(C){
								for (bright_iter = G.followSet[bright].begin(); bright_iter != G.followSet[bright].end(); bright_iter++) {
									G.followSet[right].insert(*bright_iter);
								}
							}
						}
					}
					else {    //    right元素后面没有元素  Follow(rj)+=Follow(left){
						set<string>::iterator left_iter;
						for (left_iter = G.followSet[left].begin(); left_iter != G.followSet[left].end(); left_iter++) {
							G.followSet[right].insert(*left_iter);
						}
					}
					if (esize != G.followSet[right].size())
						flag = 1;  //如果有一个非终结符的first集合的大小发生变化， 就重新遍历所有终结符
				}
			}
		}
	}
}
void calSelect() {
	for (int i = 0; i < G.expressions.size(); i++) { //遍历所有产生式
		int countEmpty = 0;			// 右端全部能推出$，则使用 Select(A-a) = First(a)-$ ∪ Follow(A)
		set<string> select;
		for (int j = 0; j < G.expressions[i].right.size(); j++) { //遍历该条产生式右边的所有元素
			string right = G.expressions[i].right[j];
			if (G.Vt.find(right) != G.Vt.end()) { // 遇到终结符
				if (right != "$")	// 不是空串，则直接加入并结束
					select.insert(right);
				else if (right == "$")
					countEmpty++;
				break;   //遇到终结符 不管这个终结符是不是空， 这条产生式的select集合就已经确定
			}
			else {// 遇到非终结符
				set<string>::iterator value_iter;   // 将其First集合中非$符加入
				for (value_iter = G.firstSet[right].begin(); value_iter != G.firstSet[right].end(); value_iter++) {
					if (*value_iter != "$")
						select.insert(*value_iter);
				}
				if (G.firstSet[right].find("$") != G.firstSet[right].end()) {
					countEmpty++;
				}
				else {
					break;
				}
			}
		}
		// 将Follow集合并入
		if (countEmpty == G.expressions[i].right.size()) {
			set<string>::iterator value_iter;
			for (value_iter = G.followSet[G.expressions[i].left].begin(); value_iter != G.followSet[G.expressions[i].left].end(); value_iter++) {
				select.insert(*value_iter);
			}
		}
		G.selectSet.push_back(select);
	}
}
int get_column(string target) { // 求终结符在预测分析表中的列标
	int i;
	set<string>::iterator t_iter;
	for (t_iter = G.Vt.begin(), i = 0; t_iter != G.Vt.end(); t_iter++, i++) {
		if (*t_iter == target)
			return i;
	}
	return 0;
}

int get_row(string target) {// 求非终结符在预测分析表中的行标
	int i;
	set<string>::iterator nt_iter;
	for (nt_iter = G.Vn.begin(), i = 0; nt_iter != G.Vn.end(); nt_iter++, i++) {
		if (*nt_iter == target)
			return i;
	}
	return 0;
}
void create_table() {
	vector<int> x;
//	cout << G.Vn.size() << " " << G.Vt.size() << endl;
	for (int i = 0; i < G.Vn.size(); i++) {
		G.table.push_back(x);
		for (int j = 0; j < G.Vt.size(); j++) {
			G.table[i].push_back(-1);
		}
	}
//	cout << G.table.size() << " " << G.table[2].size() << endl;
	for (int i = 0; i < G.expressions.size(); i++) {
		int row = get_row(G.expressions[i].left);  //获取产生式左边的非终结符造第几行 
		set<string>::iterator s_iter;
		for (s_iter = G.selectSet[i].begin(); s_iter != G.selectSet[i].end(); s_iter++) {
//			if(*s_iter=="i")
//				cout<<"YES"<<endl;
			int column = get_column(*s_iter); //获取select集中的元素在第几列 
			if (G.table[row][column] != -1) {
				ll1=0; 
				return;
			}
			else {
				G.table[row][column] = i;
			}
		}
	}
}
//void print_table() {
//	int i, j;
//	cout << "-------------预测分析表---------------" << endl << endl;
//	set<string>::iterator it;
//	vector<string>::iterator it1;
//	for (it = G.Vt.begin(); it != G.Vt.end(); it++) {
//		if (*it == "$")
//			cout << "\t" << '#';
//		else
//			cout << "\t" << *it;
//	}
//	cout << endl;
//	for (it = G.Vn.begin(), i = 0; it != G.Vn.end(); it++, i++) {
//		cout << *it;
//		for (j = 0; j < G.Vt.size(); j++) {
//			if (G.table[i][j] != -1) {
//				cout << "\t";
//				cout << G.expressions[G.table[i][j]].left << "->";
//				for (it1 = G.expressions[G.table[i][j]].right.begin(); it1 != G.expressions[G.table[i][j]].right.end(); it1++) {
//					cout << *it1 << " ";
//				}
//			}
//			else {
//				cout << "\t";
//			}
//		}
//		cout << endl;
//	}
//	cout << endl;
//}
vector<string> split(const string& str, const string& delim) {
	vector<string> res;
	if ("" == str) return res;
	//先将要切割的字符串从string类型转换为char*类型  
	char* strs = new char[str.length() + 1]; //不要忘了  
	strcpy(strs, str.c_str());

	char* d = new char[delim.length() + 1];
	strcpy(d, delim.c_str());

	char* p = strtok(strs, d);
	while (p) {
		string s = p; //分割得到的字符串转换为string类型  
		res.push_back(s); //存入结果数组  
		p = strtok(NULL, d);
	}
	delete strs;
	delete d; 
	return res;
}
void deal_tokens() {
	fstream fp("./ans.txt", ios::in);
	if (!fp) {
		cout << "文件打开失败" << endl;
		exit(-1);
	}
	string str;
	while (!fp.eof()) {         //输入token 
		getline(fp, str);
		if (str.length() == 0)
			break;
		vector<string> temp = split(str, " ");
		string token;
		if (temp[1] == "number") {
			token = temp[1];
		}
		else if (temp[1] == "operator") {
			token = temp[2];
		}
		else if (temp[1] == "limiter" || temp[1] == "keyword") {
			token = temp[2];
		}
		else if (temp[1] == "identifier") {
			token = temp[1];
		}

		tokens.push_back(token);
	}
}
int main() {
	init();
	calFisrt();
	calFollow();
	calSelect();
	create_table();
	deal_tokens();
	vector<string>::iterator it;
	int k=1;
	for(it=tokens.begin();it!=tokens.end();it++){
		cout<<k<<":"<<(*it)<<endl;
		k++;
	}
	int i, j = 0;
	if (ll1) {
		vector<string> para; 
		for(int x=0;x<tokens.size();x++){
			para.push_back(tokens[x]);
		} 
		i=para.size();
		if (para[i - 1] != "#")
			para.push_back("#");
		for(vector<string>::iterator itv=para.begin();itv!=para.end();itv++){
			cout<<*itv<<"  ";
		} 
		// 构造运算栈，将#和开始符号入栈
		stack<string> s;
		s.push("#");
		s.push(G.S);  // #S 
		// 定义读头 
		string readhead = para[j++];
		while (s.size() != 1) {  //栈底永远是# 
			string top = s.top();
			s.pop();
			if (G.Vt.find(top) != G.Vt.end()) {  //如果出栈的元素是终结符 
				if (top != "$") {  //如果栈顶的这个元素不是空$ 
					if (top == readhead)     //如果这个元素与待分析串的头部元素相同 
						readhead = para[j++];  //往下读 
					else {   //如果出现这种情况属于使用了产生式推导后产生错误  ，错误不处在源程序 
						cout<<"使用了错误的产生式导致错误！"<<endl;
						system("pause");
						return 0;
					}
				}
			}
			else {   //如果出栈的元素是非终结符 
				int row = get_row(top);
				int column = get_column(readhead);
				int index = G.table[row][column];    //table 表示预测分析表中第row行的非终结符遇到第column列的终结符时使用的是第多少号产生式 
				if (index == -1) { //预测分析表中找不到对应可用的产生式  推导不下去 
					cout<<"预测分析表中找不到对应可用的产生式"<<endl;
					system("pause");
					return 0;
				}
				else {
					int k;
					for (k = G.expressions[index].right.size() - 1; k >= 0; k--) {
						s.push(G.expressions[index].right[k]); //index号产生式右边的元素入栈 从后往前入 
					}
				}
			}
		}   //只有栈中元素被匹配完了 即S-> 一个句子 
		if (s.top() == readhead) { //如果此时输出串 还有 除了#的其它元素 即还没有被推导完 
			cout<<"语法分析成功！"<<endl;
			system("pause");
			return 0;
		}
	}
	cout<<"该文法不是ll(1)文法！"<<endl;
	system("pause");
	return 0;
}
