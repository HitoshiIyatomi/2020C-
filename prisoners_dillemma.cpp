#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <iomanip>//cin, coutのフォーマットを整えるために利用
#include <map>// 敵の手の履歴opponentHistoryを、相手毎に格納するのため。

using namespace std;

//-----------------------------------
// プレイヤーの戦略を表す列挙型
//-----------------------------------
enum Strategy {
    COOPERATION,  // 常に協調
    BETRAYAL,     // 常に裏切り
    HITBACK,      // 相手の直前の手を返す
    WATCHER,      // 相手の過去の手を見て多い手を返す
    RANDOM_STR    // ランダム
};

//-----------------------------------
// 戦略名を出力用に文字列化する関数
//-----------------------------------
string strategyToString(Strategy s) {
    switch (s) {
    case COOPERATION: return "cooperation";
    case BETRAYAL:    return "betrayal";
    case HITBACK:     return "hitback";
    case WATCHER:     return "watcher";
    case RANDOM_STR:  return "random";
    }
    return "unknown";
}

//-----------------------------------
// プレイヤーを表すクラス
//-----------------------------------
class Player {
private:
    int id;                   // 識別子
    Strategy strategy;        // 戦略
    int energy;               // エネルギー
    int lastMove;             // このプレイヤー自身が直前に出した手 (0=協調, 1=裏切り)

    // 相手ID -> 相手の過去の手(最大5回分)を保存
    // 例: opponentHistory[相手ID] = {0, 1, 0, 0, 1} (直近5回分の手)
    map<int, vector<int>> opponentHistory;

public:
    // コンストラクタ
    Player(int _id, Strategy _strategy, int _energy)
        : id(_id), strategy(_strategy), energy(_energy), lastMove(0)
    {
        // マップは自動的に空で初期化される
    }

    // プレイヤーIDを返す
    int getId() const {
        return id;
    }

    // 戦略を返す
    Strategy getStrategy() const {
        return strategy;
    }

    // エネルギーを返す
    int getEnergy() const {
        return energy;
    }

    // エネルギーを増減させる
    void addEnergy(int delta) {
        energy += delta;
    }

    // このプレイヤー自身が「直前に出した手」を返す (0=協調, 1=裏切り)
    int getLastMove() const {
        return lastMove;
    }

    // このプレイヤー自身が「直前に出した手」を更新
    void setLastMove(int move) {
        lastMove = move;
    }

    // 相手の手を履歴に追加（最大5個）
    void updateOpponentInfo(int opponentId, int oppMove) {
        vector<int>& hist = opponentHistory[opponentId];
        hist.push_back(oppMove);
        // 履歴は常に最大5件まで
        if ((int)hist.size() > 5) {
            // 先頭を削除
            hist.erase(hist.begin());
        }
    }

    // 相手IDを指定して、その履歴(最大5回分)を取得
    // 履歴が存在しない場合は空のvectorが返る
    const vector<int>& getHistoryOfOpponent(int opponentId) const {
        // map::findで検索
        map<int, vector<int>>::const_iterator it = opponentHistory.find(opponentId);
        if (it != opponentHistory.end()) {
            // 見つかった場合は、その相手の履歴を返す
            // 'second'はmapコンテナのデータ部分（要素の値）を返す
            return it->second;
        }
        else {
            // 見つからない場合は、ダミーの静的変数か何かを返す (空)
            static const vector<int> emptyVec;
            return emptyVec;
        }
    }

    // 次の手を決定する関数 (相手IDを引数で受け取る)
    int decideMove(mt19937& mt, int opponentId) {
        // 相手IDに対応する履歴を参照
        const vector<int>& historyOfOpponentMoves = getHistoryOfOpponent(opponentId);

        switch (strategy) {
        case COOPERATION:
            // 常に協調
            return 0;

        case BETRAYAL:
            // 常に裏切り
            return 1;

        case HITBACK:
            // 相手の直前の手が不明(履歴がない)場合はデフォルトで協調
            if (historyOfOpponentMoves.empty()) {
                return 0;
            }
            else {
                // 相手の直前の手を返す
                return historyOfOpponentMoves.back();
            }

        case WATCHER:
        {
            // これまでの履歴（最大5回分）で、協調が何回、裏切りが何回かを数える
            int coopCount = 0;
            int betrayCount = 0;
            for (int i = 0; i < (int)historyOfOpponentMoves.size(); i++) {
                if (historyOfOpponentMoves[i] == 0) {
                    coopCount++;
                }
                else {
                    betrayCount++;
                }
            }
            // 裏切りが多ければ裏切り、同数以下なら協調
            if (betrayCount > coopCount) {
                return 1;
            }
            else {
                return 0;
            }
        }

        case RANDOM_STR:
        {
            // 0 or 1 をランダム生成
            uniform_int_distribution<int> dist(0, 1);
            return dist(mt);
        }

        default:
            // 万一、定義外なら協調しておく
            return 0;
        }
    }
};

//-----------------------------------
// 対戦結果からエネルギーを付与する関数
//
// moveA, moveB : 0=協調, 1=裏切り
// 返り値 : (Aの得点, Bの得点) のpair
//-----------------------------------
pair<int, int> getPayoff(int moveA, int moveB) {
    // 囚人のジレンマ標準的な例:
    // 両者協調 -> (+1, +1)
    // 両者裏切り -> (-1, -1)
    // 一方が協調、他方が裏切り -> (協調: -3, 裏切り: +3)
    if (moveA == 0 && moveB == 0) {
        return make_pair(1, 1);
    }
    else if (moveA == 1 && moveB == 1) {
        return make_pair(-1, -1);
    }
    else if (moveA == 0 && moveB == 1) {
        return make_pair(-3, 3);
    }
    else { // moveA == 1 && moveB == 0
        return make_pair(3, -3);
    }
}

//-----------------------------------
// グローバル変数(パラメータ)
//-----------------------------------
int N = 100;             // 初期プレイヤー合計数
int initialEnergy = 100; // 初期エネルギー
int numEpoch = 1000;       // 対戦ラウンド数

// 各戦略の人数割合 (合計が1.0になるように設定)
double ratioCoop = 0.00;
double ratioBetray = 0.00;
double ratioHitback = 0.25;
double ratioWatcher = 0.25;
double ratioRandom = 0.50;

//-----------------------------------
// ステータス表示関数
//   - 現在生き残っているプレイヤー数と
//     戦略ごとの人数、エネルギー一覧を表示
//-----------------------------------
void status(const vector<Player>& players) {
    // 戦略ごとにプレイヤーを集計
    int countCoop = 0, countBetray = 0, countHitback = 0;
    int countWatcher = 0, countRandom = 0;

    // 各戦略のエネルギーを保存
    vector<int> energiesCoop;
    vector<int> energiesBetray;
    vector<int> energiesHitback;
    vector<int> energiesWatcher;
    vector<int> energiesRandom;

    for (int i = 0; i < (int)players.size(); i++) {
        if (players[i].getEnergy() <= 0) {
            continue;
        }
        Strategy s = players[i].getStrategy();
        int ene = players[i].getEnergy();
        switch (s) {
        case COOPERATION:
            countCoop++;
            energiesCoop.push_back(ene);
            break;
        case BETRAYAL:
            countBetray++;
            energiesBetray.push_back(ene);
            break;
        case HITBACK:
            countHitback++;
            energiesHitback.push_back(ene);
            break;
        case WATCHER:
            countWatcher++;
            energiesWatcher.push_back(ene);
            break;
        case RANDOM_STR:
            countRandom++;
            energiesRandom.push_back(ene);
            break;
        default:
            break;
        }
    }

    // 合計生存者数
    int aliveCount = countCoop + countBetray + countHitback + countWatcher + countRandom;
    cout << "-------------------------------\n";
    cout << "Current status:\n";
    cout << "  Total Alive = " << aliveCount << "\n\n";

    // 戦略ごとの情報を出力
    if (countCoop > 0) {
        cout << "  COOPERATION (" << countCoop << ")\n    Energies: ";
        for (int i = 0; i < (int)energiesCoop.size(); i++) {
            cout << energiesCoop[i] << " ";
        }
        cout << "\n";
    }
    if (countBetray > 0) {
        cout << "  BETRAYAL    (" << countBetray << ")\n    Energies: ";
        for (int i = 0; i < (int)energiesBetray.size(); i++) {
            cout << energiesBetray[i] << " ";
        }
        cout << "\n";
    }
    if (countHitback > 0) {
        cout << "  HITBACK     (" << countHitback << ")\n    Energies: ";
        for (int i = 0; i < (int)energiesHitback.size(); i++) {
            cout << energiesHitback[i] << " ";
        }
        cout << "\n";
    }
    if (countWatcher > 0) {
        cout << "  WATCHER     (" << countWatcher << ")\n    Energies: ";
        for (int i = 0; i < (int)energiesWatcher.size(); i++) {
            cout << energiesWatcher[i] << " ";
        }
        cout << "\n";
    }
    if (countRandom > 0) {
        cout << "  RANDOM      (" << countRandom << ")\n    Energies: ";
        for (int i = 0; i < (int)energiesRandom.size(); i++) {
            cout << energiesRandom[i] << " ";
        }
        cout << "\n";
    }
    cout << "-------------------------------\n\n";
}

//-----------------------------------
// メイン処理
//-----------------------------------
int main()
{
    // （疑似）乱数生成器 C++11で導入された乱数ライブラリを用いた乱数の生成
    // メルセンヌツイスタと呼ばれるアルゴリズムに基づく疑似乱数の初期化
    // rdは、ランダム値を疑似乱数のシード値（発生させるための種）にしている。
    // 19937は、2^19937 -1 のメルセンヌツイスタの乱数表の周期に由来
    //これ以降mtを使って、乱数列が得られる。
    random_device rd;
    mt19937 mt(rd());

    // 初期プレイヤーを生成
    int countCoopP = (int)(N * ratioCoop);
    int countBetrayP = (int)(N * ratioBetray);
    int countHitbackP = (int)(N * ratioHitback);
    int countWatcherP = (int)(N * ratioWatcher);
    int countRandomP = (int)(N * ratioRandom);

    // 割合の端数調整
    int sumCount = countCoopP + countBetrayP + countHitbackP + countWatcherP + countRandomP;
    if (sumCount < N) {
        countRandomP += (N - sumCount);
    }

    vector<Player> players;
    players.reserve(N);

    int currentId = 0;
    // COOPERATION
    for (int i = 0; i < countCoopP; i++) {
        players.push_back(Player(currentId++, COOPERATION, initialEnergy));
    }
    // BETRAYAL
    for (int i = 0; i < countBetrayP; i++) {
        players.push_back(Player(currentId++, BETRAYAL, initialEnergy));
    }
    // HITBACK
    for (int i = 0; i < countHitbackP; i++) {
        players.push_back(Player(currentId++, HITBACK, initialEnergy));
    }
    // WATCHER
    for (int i = 0; i < countWatcherP; i++) {
        players.push_back(Player(currentId++, WATCHER, initialEnergy));
    }
    // RANDOM
    for (int i = 0; i < countRandomP; i++) {
        players.push_back(Player(currentId++, RANDOM_STR, initialEnergy));
    }

    // numEpoch 回の対戦を行う
    for (int epoch = 0; epoch < numEpoch; epoch++) {
        // 生存プレイヤーのインデックスを集める
        vector<int> aliveIndices;
        for (int i = 0; i < (int)players.size(); i++) {
            if (players[i].getEnergy() > 0) {
                aliveIndices.push_back(i);
            }
        }

        // 生存者が1人以下なら対戦不可
        if ((int)aliveIndices.size() < 2) {
            continue;
        }

        // 各生存プレイヤーがランダムに1人選んで戦う
        for (int k = 0; k < (int)aliveIndices.size(); k++) {
            int i = aliveIndices[k];
            if (players[i].getEnergy() <= 0) {
                continue;
            }

            // ランダムに相手を1人選ぶ (自分以外)
            int jIndex;
            while (true) {
                // 0 ~ aliveIndices.size()-1 の範囲である乱数生成期distを初期化
                uniform_int_distribution<int> dist(0, (int)aliveIndices.size() - 1);
                // mtから乱数を生成して、distで制限された範囲の整数を返す　＝対戦相手 j を決める。
                jIndex = dist(mt);
                if (jIndex != k) {
                    break;
                }
            }
            int j = aliveIndices[jIndex];
            if (players[j].getEnergy() <= 0) {
                continue;
            }

            // プレイヤー i の次の手を決定 (相手は j)
            int moveI = players[i].decideMove(mt, j);
            // プレイヤー j の次の手を決定 (相手は i)
            int moveJ = players[j].decideMove(mt, i);

            // 対戦結果を反映
            pair<int, int> payoff = getPayoff(moveI, moveJ);
            players[i].addEnergy(payoff.first);
            players[j].addEnergy(payoff.second);

            // お互いの相手履歴を更新 (相手の手を記録: 最大5回分)
            players[i].updateOpponentInfo(j, moveJ);
            players[j].updateOpponentInfo(i, moveI);

            // 自分の「直前の手」を更新 (こちらは全体で1つだけ持つ)
            players[i].setLastMove(moveI);
            players[j].setLastMove(moveJ);
        }
    }

    // 対戦終了後に結果表示
    status(players);

    return 0;
}


