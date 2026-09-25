#include "replay_controller.hpp"

#include <QVariantMap>
#include <algorithm>

ReplayController::ReplayController(QObject* parent) : QObject(parent) {}
QVariantList ReplayController::stones() const { QVariantList result; for (int y=0;y<board_.size();++y) for(int x=0;x<board_.size();++x) { const auto stone=board_.at(x,y); if(stone==weiqi::Stone::empty) continue; QVariantMap point; point["x"]=x; point["y"]=y; point["black"]=stone==weiqi::Stone::black; result.append(point); } return result; }
void ReplayController::loadSgf(const QString& sgf) { try { const auto text=sgf.toStdString(); for (const int size : {19,13,9}) { try { moves_=weiqi::parse_sgf_moves(text,size); board_size_=size; break; } catch (...) {} } if (moves_.empty() && text.find(";B[")==std::string::npos) throw std::invalid_argument("SGF 中没有可回放的着法"); step_=0; error_.clear(); rebuild(); } catch (const std::exception& e) { error_=QString::fromUtf8(e.what()); emit changed(); } }
void ReplayController::previous() { jumpTo(step_-1); }
void ReplayController::next() { jumpTo(step_+1); }
void ReplayController::jumpTo(int value) { step_=std::clamp(value,0,static_cast<int>(moves_.size())); rebuild(); }
void ReplayController::rebuild() { try { board_=weiqi::GoBoard(board_size_); for(int i=0;i<step_;++i) { const auto& move=moves_[i]; if(board_.current_player()!=move.color) throw std::invalid_argument("SGF 行棋顺序无效"); if(move.x<0) board_.pass(); else if(!board_.play(move.x,move.y).accepted) throw std::invalid_argument("SGF 包含非法着法"); } error_.clear(); } catch(const std::exception& e) { error_=QString::fromUtf8(e.what()); } emit changed(); }
