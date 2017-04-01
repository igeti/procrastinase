#include <string>
#include <memory>

class ForeignWindow {
public:
	void kill()
	std::string getWindowTitle();
	std::string getProgramPath();
private:
	std::unique_ptr<ForeignWindowImpl> impl;
}
