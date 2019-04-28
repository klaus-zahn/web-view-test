#include <iostream>
using namespace std;

#include "game.h"
#include "includes.h"

CGame::CGame():m_num_moves(0)
{
  m_fields.insert(map<string, bool>::value_type("A1", false));
  m_fields.insert(map<string, bool>::value_type("A2", false));
  m_fields.insert(map<string, bool>::value_type("A3", false));
  m_fields.insert(map<string, bool>::value_type("B1", false));
  m_fields.insert(map<string, bool>::value_type("B2", false));
  m_fields.insert(map<string, bool>::value_type("B3", false));
  m_fields.insert(map<string, bool>::value_type("C1", false));
  m_fields.insert(map<string, bool>::value_type("C2", false));
  m_fields.insert(map<string, bool>::value_type("C3", false));

}


void CGame::setField(string field_name)
{
  map<string, bool>::iterator it = m_fields.find(field_name);
  if(it != m_fields.end()) {
    it->second = true;
  } else {
    OscLog(ERROR, "The field name %s does not exists\n", field_name.c_str());
  }
}


std::string CGame::getNextField()
{
    string ret_val;

    if(m_num_moves == 5) {
      OscLog(ERROR, "The maximum number of moves is reached. You have to reset the game.\n");
      return ret_val;
    }

    map<string, bool>::iterator it = m_fields.begin();
    bool go = true;
    for(; go && it != m_fields.end() ; it++) {
      if(it->second == false) {
        ret_val = it->first;
        go = false;
      }
    }
    m_num_moves++;
    return ret_val;
}


void CGame::reset_game()
{
  m_num_moves = 0;

  map<string, bool>::iterator it = m_fields.begin();
  for(; it != m_fields.end(); it++) {
    it->second = false;
  }
}
