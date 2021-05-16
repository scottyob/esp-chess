#ifndef CHESS_H
#define CHESS_H

#include <stdio.h>
#include <string>
#include <vector>
#include "thc.h"
#include "table.h"
#include <string>

#define CHESSBOARD_SIZE 8

/*
   Chess is the engine which drives the interaction between the table
   and the chess library.
   It can process and provide updates in JSON format as follows (with comments)
   {
       // Increases with each move, allows us to compare remote game state with our own
       "halfMoveCount": 0,
       "fen": "",
       "previousFen": "",
       "isWhite": 0,
       "remotePlayer": "",
       "history": [
         {
           "src": "a2",
           "dst": "a3"
         }
       ]
   }
*/

/*
   A move in a history of the game
*/
struct Move {
  String src;
  String dst;
};

/*
   The state of the game
*/
struct ChessState {
  long halfMoveCount;
  String fen;
  String previousFen;
  bool isWhite;
  String remotePlayer;
  std::vector<Move> history;
  bool fromRemote;
};

/*
   Driver for interfacing with the board,
   network & chess libraries.
*/
class Chess {
  private:
    Table* table;
    thc::Square holding = thc::Square::SQUARE_INVALID;
    void (*messageCallback)(const String &qr, const String &message);
    thc::ChessRules startState;
  public:
    bool needsPublishing;
    Chess(Table* table) {
      this->table = table;
      needsPublishing = false;
      messageCallback = NULL;
    }
    ChessState gameState = {};
    thc::ChessRules cr;
    thc::ChessRules previousChessGame;
    void update(std::vector<std::string> history);
    void didChange();
    void didUpdate();
    std::vector<thc::Square> findDeltas() {
      return findDeltas(cr);
    }
    std::vector<thc::Square> findDeltas(const thc::ChessRules &c);
    void updateRecieved(const ChessState &remote, const bool &forceReplace);
    void onMessage(void(* callback)(const String &qr, const String &message)) {
      this->messageCallback = callback;
    }
};

#endif
