#include "chess.h"

// Finds the squares that are different to the board.
std::vector<thc::Square> Chess::findDeltas(const thc::ChessRules &c) {
  std::vector<thc::Square> ret;

  char populated[65];
  table->populateSquares(populated);

  for (int i = thc::Square::a8; i < thc::Square::SQUARE_INVALID; i++) {
    auto isFilled = ((BoardLocation_T*)&table->board)[i].filled;
    if (isFilled != (c.squares[i] != ' ')) {
      ret.push_back(static_cast<thc::Square>(i));
    }
  }
  return ret;
}

void Chess::didUpdate(const bool &sleeping) {
  if (!sleeping) {
    sleepAt = millis() + MINUTES_30;
  }

  Serial.println("Debugging, game state");
  Serial.println(cr.ToDebugStr().c_str());

  auto deltas = findDeltas();
  int colors[GRID_SIZE * GRID_SIZE] = {BoardColor::NONE};
  bool renderDeltaColors = true;

  // Figure out all of the possible moves that can be made
  std::vector<thc::Move> possibleMoves;
  cr.GenLegalMoveList(possibleMoves);


  // If we're at a game-over state.  Draw the winner gold
  // as a base layer
  thc::TERMINAL endGame;
  cr.Evaluate(endGame);
  if (endGame == thc::TERMINAL_BCHECKMATE) {
    for (int row = 6; row < 8; row++)
      for (int col = 0; col < 8; col++) {
        colors[row * 8 + col] = BoardColor::GOLD;
      }
  } else if (endGame == thc::TERMINAL_WCHECKMATE) {
    for (int row = 0; row < 2; row++)
      for (int col = 0; col < 8; col++) {
        colors[row * 8 + col] = BoardColor::GOLD;
      }
  } else if (endGame != thc::NOT_TERMINAL) {
    for (int row = 0; row < 2; row++)
      for (int col = 0; col < 8; col++) {
        colors[row * 8 + col] = BoardColor::GOLD;
      }
    for (int row = 6; row < 8; row++)
      for (int col = 0; col < 8; col++) {
        colors[row * 8 + col] = BoardColor::GOLD;
      }
  }

  if (endGame != thc::NOT_TERMINAL) {
    // Ability to restart the game if the pieces are
    // back in starting position
    if (findDeltas(startState).size() == 0) {
      // we are at the starting position.  Want a new game
      gameState.halfMoveCount = 0;
      gameState.fen = "";
      gameState.previousFen = "";
      gameState.isWhite = !gameState.isWhite;
      gameState.history = "";
      needsPublishing = true;
      return;
    }
  }


  if (
    holding != thc::Square::SQUARE_INVALID
    && deltas.size() == 0
  ) {
    // A piece we were holding we put back.
    holding = thc::Square::SQUARE_INVALID;
    auto deltas = findDeltas();
  }
  else if (
    holding == thc::Square::SQUARE_INVALID
    && deltas.size() == 1
    && cr.WhiteToPlay() == gameState.isWhite)
  {
    // We have picked up a piece.  We were not before
    auto deltaSquare = deltas.front();
    colors[static_cast<int>(deltaSquare)] = BoardColor::GREEN;

    for (auto &move : possibleMoves) {
      if (move.src != deltaSquare)
        continue;
      colors[static_cast<int>(move.dst)] = BoardColor::LIGHTGREEN;
      // There are valid moves, so don't render red difference
      renderDeltaColors = false;
      holding = deltaSquare;
    }
  } else if (holding != thc::Square::SQUARE_INVALID) {
    if (deltas.size() == 2) {
      // A move has been made.  Figure out which possible one it is.
      auto target = deltas.front();
      if (target == holding)
        target = deltas.back();

      for (auto &move : possibleMoves) {
        if (move.src == holding && move.dst == target) {
          // This is a valid move.  Make it.
          // Add the Portable Game Notation format move to our move string.
          gameState.history += String(" ") + String(move.NaturalOut(&cr).c_str());

          cr.PlayMove(move);
          holding = thc::Square::SQUARE_INVALID;
          deltas = findDeltas();

          //Update our move to the network & state object.
          gameState.halfMoveCount++;
          gameState.previousFen = gameState.fen;
          gameState.fen = String(cr.ForsythPublish().c_str());

          needsPublishing = true;
        }
      }
    }
  }

  // If it's our turn, and the deltas match that of the previous
  // board state, then show the last move src, dst.
  if (findDeltas(previousChessGame).size() == 0) {
    // Load a list of all the possible moves from the previous game state.
    // Find out which move was responsible to get it to the current state.
    // Highlight the src + dst colors on the board.
    std::vector<thc::Move> previousPossibleMoves;
    previousChessGame.GenLegalMoveList(previousPossibleMoves);

    for (auto &move : previousPossibleMoves) {
      // Create a new board with this move made
      thc::ChessRules possibleState = previousChessGame;
      possibleState.PlayMove(move);
      if (possibleState == cr) {
        colors[static_cast<int>(move.src)] = BoardColor::GREEN;
        colors[static_cast<int>(move.dst)] = BoardColor::LIGHTGREEN;
        renderDeltaColors = false;
      }
    }
  }

  if (renderDeltaColors) {
    // Renders all deltas read
    for (auto & coord : deltas) {
      colors[static_cast<int>(coord)] = BoardColor::RED;
    }
  }
  table->render((const int(*)[8])&colors, 255, sleeping);

  // Update the
  if (messageCallback) {
    // Lob off the first line "x to move"
    String state = cr.ToDebugStr().c_str();
    auto newln = state.indexOf('\n', 1);
    Serial.println("Newline found at");
    Serial.println(newln);
    String stateSmall = String((char*)&state.c_str()[newln + 1]);
    messageCallback("", stateSmall);
  }

}

void Chess::loop() {
  // General game loop.  Takes care of sleeping the board after no activity.
  if (sleepAt > 0 && millis() > sleepAt) {
    sleepAt = 0;
    didUpdate(true); //Re-render the board in sleep state
  }
}

/*
   When an update has been recieved, update our local state to match
   if need be.
*/
void Chess::updateRecieved(const ChessState &remote, const bool &forceReplace) {
  if (!gameState.fromRemote) {
    gameState = remote;
  } else if (
    remote.halfMoveCount <= gameState.halfMoveCount
    && !forceReplace
  ) {
    return;  // Not newer
  }

  if (gameState.fen != remote.fen) {
    needsPublishing = true; // Update our local shadow with updated remote position.
  }

  // Load our local state to match the remote
  gameState.halfMoveCount = remote.halfMoveCount;
  gameState.fen = remote.fen;
  gameState.previousFen = remote.previousFen;
  gameState.history = remote.history;

  //If there's blank items, then replace the board properly
  String starting_fen = String("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  if (gameState.fen == "") {
    gameState.fen = starting_fen;
  }
  if (gameState.previousFen == "") {
    gameState.previousFen = starting_fen;
  }


  // Update our local chess instante to the new fen
  auto success = cr.Forsyth(gameState.fen.c_str());
  previousChessGame.Forsyth(gameState.previousFen.c_str());

  // Update the game.
  didUpdate(false);
}
