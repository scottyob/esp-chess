#include "chess.h"

// Finds the squares that are different to the board.
std::vector<thc::Square> Chess::findDeltas(const thc::ChessRules &c)
{
  std::vector<thc::Square> ret;

  char populated[65];
  table->populateSquares(populated);

  for (int i = thc::Square::a8; i < thc::Square::SQUARE_INVALID; i++)
  {
    auto isFilled = ((BoardLocation_T *)&table->board)[i].filled;
    if (isFilled != (c.squares[i] != ' '))
    {
      ret.push_back(static_cast<thc::Square>(i));
    }
  }
  return ret;
}

void Chess::redrawBoard(const bool &sleeping)
{
  if (!sleeping)
  {
    sleepAt = millis() + MINUTES_30;
  }

  Serial.println("Debugging, game state");
  dumpChessState(gameState);
  Serial.println(cr.ToDebugStr().c_str());

  auto deltas = findDeltas();
  int colors[GRID_SIZE * GRID_SIZE] = {BoardColor::NONE};
  bool renderDeltaColors = true;

  // Figure out all of the possible moves that can be made
  std::vector<thc::Move> possibleMoves;
  cr.GenLegalMoveList(possibleMoves);

  // If we're on a brand new game, but the pieces are in the same
  // position from the previous game, then highlight the deltas
  if (
      gameState.sequenceNumber == 0 && (findDeltas(previousGameLastState).size() == 0 || findDeltas(previousGamePreviousMoveState).size() == 0))
  {
    thc::TERMINAL endGame;
    previousGameLastState.Evaluate(endGame);

    highlightMoveMade(colors, previousGamePreviousMoveState, previousGameLastState);

    if (endGame == thc::TERMINAL_BCHECKMATE)
    {
      for (int row = 6; row < 8; row++)
        for (int col = 0; col < 8; col++)
          colors[row * 8 + col] = BoardColor::GOLD;
    }
    else if (endGame == thc::TERMINAL_WCHECKMATE)
    {
      for (int row = 0; row < 2; row++)
        for (int col = 0; col < 8; col++)
          colors[row * 8 + col] = BoardColor::GOLD;
    }
    else if (endGame != thc::NOT_TERMINAL)
    {
      for (int row = 0; row < 2; row++)
        for (int col = 0; col < 8; col++)
          colors[row * 8 + col] = BoardColor::GOLD;
      for (int row = 6; row < 8; row++)
        for (int col = 0; col < 8; col++)
          colors[row * 8 + col] = BoardColor::GOLD;
    }

    table->render((const int(*)[8]) & colors, 255, sleeping);
    return;
  }

  if (
      holding != thc::Square::SQUARE_INVALID && deltas.size() == 0)
  {
    // A piece we were holding we put back.
    holding = thc::Square::SQUARE_INVALID;
    auto deltas = findDeltas();
  }
  else if (
      holding == thc::Square::SQUARE_INVALID && deltas.size() == 1 && cr.WhiteToPlay() == gameState.isWhite)
  {
    // We have picked up a piece.  We were not before
    auto deltaSquare = deltas.front();
    colors[static_cast<int>(deltaSquare)] = BoardColor::GREEN;

    for (auto &move : possibleMoves)
    {
      if (move.src != deltaSquare)
        continue;
      colors[static_cast<int>(move.dst)] = BoardColor::LIGHTGREEN;
      // There are valid moves, so don't render red difference
      renderDeltaColors = false;
      holding = deltaSquare;
    }
  }
  else if (holding != thc::Square::SQUARE_INVALID)
  {
    if (deltas.size() == 2)
    {
      // A move has been made.  Figure out which possible one it is.
      auto target = deltas.front();
      if (target == holding)
        target = deltas.back();

      for (auto &move : possibleMoves)
      {
        if (move.src == holding && move.dst == target)
        {
          playMove(move);
          deltas = findDeltas();
          needsPublishing = true;
        }
      }
    }
  }

  // If it's our turn, and the deltas match that of the previous
  // board state, then show the last move src, dst.
  if (findDeltas(previousMoveChessGame).size() == 0)
  {
    renderDeltaColors = !highlightMoveMade(colors, previousMoveChessGame, cr);
  }

  if (renderDeltaColors)
  {
    // Renders all deltas read
    for (auto &coord : deltas)
    {
      colors[static_cast<int>(coord)] = BoardColor::RED;
    }
  }
  table->render((const int(*)[8]) & colors, 255, sleeping);

  // Update the
  if (messageCallback)
  {
    // Lob off the first line "x to move"
    String state = cr.ToDebugStr().c_str();
    auto newln = state.indexOf('\n', 1);
    Serial.println("Newline found at");
    Serial.println(newln);
    String stateSmall = String((char *)&state.c_str()[newln + 1]);
    messageCallback("", stateSmall);
  }
}

void Chess::loop()
{
  // General game loop.  Takes care of sleeping the board after no activity.
  if (sleepAt > 0 && millis() > sleepAt)
  {
    sleepAt = 0;
    redrawBoard(true); //Re-render the board in sleep state
  }

  // Redraw the board if there's been network or local board activity
  if (
      lastActivityDrawn != table->lastActivity || lastDrawnSequenceNumber != gameState.sequenceNumber)
  {
    redrawBoard(false);
    lastActivityDrawn = table->lastActivity;
    lastDrawnSequenceNumber = gameState.sequenceNumber;
  }
}

/*
   When an update has been recieved, update our local state to match
   if need be.
*/
void Chess::updateRecieved(const ChessState &newState, const bool &remotePlayer)
{
  // Make sure we update the remote player to that of the shadow service
  // regardless of sequence numbers
  if (!remotePlayer) {
    // If we've got an update from our board, make sure we're no longer holding a piece
    holding = thc::Square::SQUARE_INVALID;
    gameState.remotePlayer = newState.remotePlayer;
  }

  // Avoid updating if the game's sequence number is lower than ours
  // Sequence number of 0 signifies a new game and trumps everything
  if (
      newState.sequenceNumber != 0 && newState.sequenceNumber < gameState.sequenceNumber)
  {
    return;
  }

  if (gameState.fen != newState.fen)
  {
    needsPublishing = true; // Update our local shadow with updated newState position.
  }

  // We're always the opposite of the opponents white/black.
  if (remotePlayer)
    gameState.isWhite = !newState.isWhite;
  else
    gameState.isWhite = newState.isWhite;

  // Load our local state to match the newState
  gameState.sequenceNumber = newState.sequenceNumber;
  gameState.fen = newState.fen;
  gameState.previousFen = newState.previousFen;
  gameState.history = newState.history;
  gameState.lastGameFen = newState.lastGameFen;
  gameState.lastGamePreviousFen = newState.lastGamePreviousFen;

  // If there's blank items in FENs, then replace with the a new game FEN
  String starting_fen = String("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  if (gameState.fen == "")
    gameState.fen = starting_fen;
  if (gameState.previousFen == "")
    gameState.previousFen = starting_fen;
  if (gameState.lastGameFen == "")
    gameState.lastGameFen = starting_fen;
  if (gameState.lastGamePreviousFen == "")
    gameState.lastGamePreviousFen = starting_fen;

  // Update our local chess instante to the new fen
  auto success = cr.Forsyth(gameState.fen.c_str());
  previousMoveChessGame.Forsyth(gameState.previousFen.c_str());
  previousGameLastState.Forsyth(gameState.lastGameFen.c_str());
  previousGamePreviousMoveState.Forsyth(gameState.lastGamePreviousFen.c_str());

  // Update the game.
  // redrawBoard(false);
}

/*
    Highlight the move that was made to transition between the two chess states.  Update the color map
    to reflect this move.
*/
bool Chess::highlightMoveMade(int colors[], thc::ChessRules &previousState, thc::ChessRules &currentState)
{
  std::vector<thc::Move> previousPossibleMoves;
  previousState.GenLegalMoveList(previousPossibleMoves);

  for (auto &move : previousPossibleMoves)
  {
    // Create a new board with this move made
    thc::ChessRules possibleState = previousState;
    possibleState.PlayMove(move);
    if (possibleState == currentState)
    {
      colors[static_cast<int>(move.src)] = BoardColor::GREEN;
      colors[static_cast<int>(move.dst)] = BoardColor::LIGHTGREEN;
      return true;
    }
  }

  return false;
}

/*
    A move has been made by the local player.  Update the state machine to reflect the new current move
    The previous move, and sequence number, starting a new game if required.
*/
void Chess::playMove(thc::Move &move)
{
  // Add the Portable Game Notation format move to our move string.
  cr.PlayMove(move);
  holding = thc::Square::SQUARE_INVALID;

  // Update our move to the network & state object.
  gameState.sequenceNumber++;
  gameState.previousFen = gameState.fen;
  gameState.fen = String(cr.ForsythPublish().c_str());
  gameState.history += String(" ") + String(move.NaturalOut(&cr).c_str());

  // Check to see if the game is over
  thc::TERMINAL endGame;
  cr.Evaluate(endGame);

  // Short-circuit if we're not in an end-game scenario
  if (endGame == thc::NOT_TERMINAL)
    return;

  // The game has finished.  Start a new one
  gameState.lastGameFen = gameState.fen;
  gameState.lastGamePreviousFen = gameState.previousFen;
  gameState.sequenceNumber = 0;
  gameState.fen = "";
  gameState.previousFen = "";
  gameState.isWhite = !gameState.isWhite; // Swap the player, alternate who plays white
  gameState.history = "";
}

/*
 * Debug statement to print the current chess state
 */
void dumpChessState(const ChessState &s) {
  Serial.print("sequenceNumber: ");
  Serial.println(s.sequenceNumber);
  Serial.print("fen: ");
  Serial.println(s.fen);
  Serial.print("previousFen: ");
  Serial.println(s.previousFen);
  Serial.print("isWhite: ");
  Serial.println(s.isWhite);
  Serial.print("remotePlayer: ");
  Serial.println(s.remotePlayer);
  Serial.print("history: ");
  Serial.println(s.history);
  Serial.print("lastGameFen: ");
  Serial.println(s.lastGameFen);
  Serial.print("lastGamePreviousFen: ");
  Serial.println(s.lastGamePreviousFen);
}
