#include <unordered_set>
#include <unordered_map>

int main(int argc, char **argv) {
  const std::unordered_map<int,int> a;
  const std::unordered_set<int> b;
  a.find(1);
  b.find(1);
}
