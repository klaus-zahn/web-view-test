/*! @file game.h
 * @brief class for game strategy
 */


#ifndef GAME_H_
#define GAME_H_

#include <map>
#include <string>


class CGame {
public:
  CGame();

  ~CGame() {}

  void setField(std::string field_name);

  std::string getNextField();

  int getMove() {return m_num_moves;}

  void reset_game();

private:

  std::map<std::string, bool> m_fields;

  int m_num_moves;
};



#endif //GAME_H_

