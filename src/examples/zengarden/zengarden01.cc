/*
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2025 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 *
 *  \see https://github.com/morinim/ultra/wiki/zen_garden_puzzle
 */

#include <iostream>
#include <set>
#include <vector>

#include "kernel/ultra.h"

enum cell {Empty = ' ', Perimeter = 'X', Rock = '*', Ornament = '^',
           Yellow = 'Y', Orange = 'O', Red = 'R'};

struct position
{
  unsigned row;
  unsigned col;
};

enum cardinal_dir {north, south, west, east};

class zen_garden
{
public:
  zen_garden(unsigned rows, unsigned cols, const std::set<position> &rocks)
    : garden_(rows + 2, cols + 2)  // +2 is needed to fit the perimeter
  {
    for (unsigned row(0); row < rows(); ++row)
      for (unsigned col(0); col < columns(); ++col)
        if (row == 0 || row+1 == rows()
            || col == 0 || col+1 == columns())
          garden_(row, col) = Perimeter;
        else
          garden_(row, col) = Empty;

    for (const auto &pos : rocks)
      garden_(pos) = Rock;
  }

  cardinal_dir direction_at(position pos) const
  {
    if (pos.row == 0)
      return south;

    if (pos.col+1 == columns())
      return west;

    if (pos.row+1 == rows())
      return north;

    assert(pos.col == 0);
    return east;
  }

  // X 0 1 2 X
  //11       3
  //10       4
  // 9       5
  // X 8 7 6 X
  position index_to_pos(unsigned i) const
  {
    assert(i < perimeter());

    if (i < columns()-2)
      return {0, i+1};

    if (i < columns()-2 + rows()-2)
      return {i + 3 - columns(), columns()-1};

    if (i < columns()-2 + rows()-2 + columns()-2)
      return (rows()-1, 2*columns() + rows() - i - 6};

    return {perimeter() - i,  0};
  }

  unsigned perimeter() const
  {
    return 2*(rows() + cols()) - 8;
  }

private:
  ultra::matrix<cell> garden_;
};

string getPermutation(int n, int k) {
    int i,j,f=1;
    // left part of s is partially formed permutation, right part is the leftover chars.
    string s(n,'0');
    for(i=1;i<=n;i++){
        f*=i;
        s[i-1]+=i; // make s become 1234...n
    }
    for(i=0,k--;i<n;i++){
        f/=n-i;
        j=i+k/f; // calculate index of char to put at s[i]
        char c=s[j];
        // remove c by shifting to cover up (adjust the right part).
        for(;j>i;j--)
            s[j]=s[j-1];
        k%=f;
        s[i]=c;
    }
    return s;
}

int main()
{
  using namespace ultra;

  const std::set<position> rocks =
  {
    {4, 6}, {7, 3}, {2, 10}, {2, 4}, {7, 9}, {7, 10}
  };

  zen_garden garden(10, 12, rocks);

  ga::problem prob;

  //
  // Gene format:
  // {i,  // i-th permutation of [0, 1, 2... perimeter()-1]
  //  choice_01, choice_02, ... choice_0M,  // sequence of choice in position 0
  //  choice_11, choice_12, ... choice_1M,  // sequence of choice in position 1
  //  ...
  //  choice_N1, choice_N2, ... choice_NM}  // sequence of choice in position N
  //
  prob.insert({0, garden.perimeter()});  // starting position
  for (unsigned i(0); i < std::max(garden.rows(), garden.cols()); ++i)
    prob.insert({0, 2});                 // choice at the i-th obstacle

}

/*
cell_coord update_coord(const maze &m, cell_coord start, cardinal_dir d)
{
  Expects(d == north || d == south || d == west || d == east);
  auto to(start);

  switch(d)
  {
  case north:
    if (start.first > 0)
      --to.first;
    break;

  case south:
    if (start.first + 1 < m.size())
      ++to.first;
    break;

  case west:
    if (start.second > 0)
      --to.second;
    break;

  default:
    if (start.second + 1 < m[0].size())
      ++to.second;
  }

  return m[to.first][to.second] == Empty ? to : start;
}

std::vector<cell_coord> extract_path(const ultra::ga::individual &dirs,
                                     const maze &m,
                                     cell_coord start, cell_coord goal)
{
  std::vector<cell_coord> ret;

  cell_coord now(start);

  for (unsigned i(0); i < dirs.parameters() && now != goal; ++i)
  {
    const auto dir(dirs[i]);
    cell_coord prev;
    do
    {
      prev = now;
      ret.push_back(now);
      now = update_coord(m, now, cardinal_dir(dir));
    } while (now != prev && now != goal && !crossing(m, now));

    if (now == goal)
      ret.push_back(goal);
  }

  return ret;
}

std::pair<cell_coord, unsigned> run(const ultra::ga::individual &dirs,
                                    const maze &m,
                                    cell_coord start, cell_coord goal)
{
  const auto path(extract_path(dirs, m, start, goal));

  return {path.back(), path.size()};
}

void print_maze(const maze &m)
{
  const std::string hr(m[0].size() + 2, '-');

  std::cout << hr << '\n';

  for (const auto &rows : m)
  {
    std::cout << '|';

    for (const auto &cell : rows)
      std::cout << cell;

    std::cout << "|\n";
  }

  std::cout << hr << '\n';
}

maze path_on_maze(const std::vector<cell_coord> &path, maze base,
                  cell_coord goal)
{
  for (const auto &c : path)
    base[c.first][c.second] = '.';

  base[path.front().first][path.front().second] = Start;

  if (path.back() == goal)
    base[goal.first][goal.second] = Goal;

  return base;
}

int main()
{
  using namespace ultra;

  prob.insert({0, perimeter(garden)});
  for (unsigned i(0); i

  const auto length(m.size() * m[0].size() / 2);

  // A candidate solution is a sequence of `length` integers each representing
  // a cardinal direction.
  ga::problem prob(length, {0, 4});

  prob.params.population.individuals = 150;
  prob.params.evolution.generations  =  20;

  auto f = [m, start, goal](const ultra::ga::individual &x)
  {
    const auto final(run(x, m, start, goal));

    return -distance(final.first, goal) - final.second / 1000.0;
  };

  ga::search search(prob, f);

  const auto best_path(extract_path(search.run().best_individual, m, start,
                                    goal));

  print_maze(path_on_maze(best_path, m, goal));
  }*/
