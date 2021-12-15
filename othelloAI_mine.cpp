#include<iostream>
#include<vector>
#include <time.h>
#include<math.h>
typedef long long ll;
#define rep(i,n) for(int i=0;i<(int)n;i++)
template<class T> inline bool chmax(T& a, T b) { if (a < b) { a = b; return 1; } return 0; }
template<class T> inline bool chmin(T& a, T b) { if (a > b) { a = b; return 1; } return 0; }
using namespace std;
vector<vector<int>> table(8, vector<int>(8, -1));
int dy[8] = { 1,1,0,-1,-1,-1,0,1 }, dx[8] = { 0,1,1,1,0,-1,-1,-1 };
struct State {
    int my;                                  //自分の駒の色 0or1
    vector<vector<int>> t;

    //コンストラクタ
    State() {
        t.assign(8, vector<int>(8, -1));
        init();
        my = 0;
    }

    State(int my_, vector<vector<int>> t_) {
        my = my_;
        t = t_;
    }

    //0:not-end 1:lose 2:draw
    int is_done() {
        int cou = 0;
        rep(i, 8)rep(j, 8)if (t[i][j] == -1)return 0;
        else if (t[i][j] == my)cou++;
        if (cou == 32)return 2;
        else if (cou > 32)return 1;
        return 0;
    }

    bool dfsrev(vector<vector<int>>& nt, int y, int x, int k, int cou = 0) {
        if (0 > y || y >= 8 || 0 > x || x >= 8)return false;
        int ny = y + dy[k], nx = x + dx[k];
        if (0 > ny || ny >= 8 || 0 > nx || nx >= 8 || nt[ny][nx] == -1)return false;
        if (nt[ny][nx] == my) {
            if (cou > 0) {
                nt[y][x] = my;
                return true;
            }
            else return false;
        }
        if (dfsrev(nt, ny, nx, k, cou + 1)) {
            nt[y][x] = my;
            return true;
        }
        else return false;
    }

    State next(pair<int, int> p) {
        int y = p.first, x = p.second;
        State newState(1 - my, t);
        
        //打つ手なしでパスの場合
        if(y==-1&&x==-1){
            return newState;
        }
        
        auto& nt = newState.t;
        nt[y][x] = my;
        rep(k, 8) {
            //int ny=y+dy[k],nx=x+dx[k];
            dfsrev(nt, y, x, k);
        }
        newState.t = nt;
        return newState;
    }

    vector<pair<int, int>> legal_actions() {
        vector<pair<int, int>> vp;
        rep(y, 8)rep(x, 8) {
            auto nt = t;
            if (nt[y][x] != -1)continue;
            nt[y][x] = my;
            rep(k, 8)if (dfsrev(nt, y, x, k)) {
                vp.push_back({ y,x });
                break;
            }
        }
        return vp;
    }

    void show() {
        auto ct=t;
        auto la=legal_actions();
        for(auto legal_action:la)ct[legal_action.first][legal_action.second]=2;
        rep(i, 8) {
            rep(j, 8) {
                if (ct[i][j] == 0)cout << 'o';
                else if (ct[i][j] == 1)cout << 'x';
                else if (ct[i][j] == 2)cout << '!';
                else cout << '-';
            }
            cout << endl;
        }
        cout << endl;
        //for(auto legal_action:la)cout<<legal_action.first<<" "<<legal_action.second<<endl;
        //cout<<endl;
        return;
    }

    void init() {
        rep(i, 8)rep(j, 8)t[i][j] = -1;
        t[3][3] = t[4][4] = 0;
        t[3][4] = t[4][3] = 1;
        return;
    }




};


struct mcts_action {
    State self_state;
    int self_w;
    int self_n;
    vector<mcts_action> self_child_nodes;

    //コンストラクタ
    mcts_action(State s) {
        self_state = s;
        self_w = 0; self_n = 0;
    }

    int evaluate() {
        //ゲーム終了時
        int result = self_state.is_done();
        //負け
        if (result == 1)return -1;     //負け
        else if (result == 2)return 0; //引き分け
        

        //子ノードが存在しない時
        if (self_child_nodes.size() == 0) {
            
            //プレイアウト
            int value = playout();

            //累計価値と試行回数の更新
            self_w += value;
            self_n++;

            //展開
            if (self_n == 10) {
                expand();
            }
            return value;
        }
        else {  //子ノードが存在するとき
            
            int value=-next_child_node().evaluate();
            
            //累計価値と試行回数の更新
            self_w+=value;
            self_n++;
            return value;
        }
        return 100;
    }

    int playout() {
        int value = 0;
        //ゲーム終了時
        int result = self_state.is_done();
        //負け
        if (result == 1)return -1;     //負け
        else if (result == 2)return 0; //引き分け

        auto legal_action = self_state.legal_actions();
        
        //打つ手なしの場合もある
        if(legal_action.size()==0)legal_action.push_back({-1,-1});
        int rdm = rand() % legal_action.size();
        State nextS2 = self_state.next(legal_action[rdm]);
        mcts_action nextM(nextS2);
        value += nextM.playout();
        return value;
    }

    mcts_action next_child_node() {
        double t = 0;
        //試行回数が0の子ノードを返す
        for (auto child_node : self_child_nodes) {
            t += child_node.self_n;
            if (child_node.self_n == 0)return child_node;
        }

        //UCB1の計算
        double maxUCB = 0.0;
        mcts_action res(self_state);
        for (auto child_node : self_child_nodes) {
            double UCB = sqrt(2 * log(t) / child_node.self_n) - child_node.self_w / child_node.self_n;
            if (chmax(maxUCB, UCB))res = child_node;
        }
        return res;
    }

    //展開
    void expand() {
        auto L_actions = self_state.legal_actions();
        //cerr<<"legal_actions.size(): "<<L_actions.size()<<endl;
        for (auto& legal_action : L_actions) {
            State nextS = self_state.next(legal_action);
            mcts_action nextM(nextS);
            self_child_nodes.push_back(nextM);
        }
        return;
    }

    pair<int, int> select() {
        //expand();
        rep(_, 100) evaluate();

        //試行回数の最大値を持つ行動を返す
        auto L_actions = self_state.legal_actions();
        int maxVal = -10000, idx = -1;
        mcts_action res(self_state);
        rep(i, self_child_nodes.size()) {
            if (chmax(maxVal, self_child_nodes[i].self_w))idx = i;
        }
        return L_actions[idx];

    }

};


int main(void) {
    State game;
    game.show();
    while (game.is_done() == 0) {
        pair<int, int> p={-1,-1};
        if (game.my == 1) {
            mcts_action m(game);
            p = m.select();
        }
        else {
            mcts_action m(game);
            p = m.select();
        }
        game = game.next(p);
        
        game.show();
    }
}


