#include <tr1/unordered_set>
#include <tr1/unordered_map>

int main(int argc, char **argv) {
  const std::tr1::unordered_map<int,int> a;
  const std::tr1::unordered_set<int> b;
  // make sure that the tr1 const find_node bug is fixed.
  a.find(1);
  b.find(1);
}
