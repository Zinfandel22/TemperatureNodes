#pragma once

class Preferences {
public:
    bool hasHub = true;
    bool removeSucceeds = true;
    unsigned removals = 0;

    bool isKey(const char *) const { return hasHub; }
    bool remove(const char *) {
        ++removals;
        if (!removeSucceeds) return false;
        hasHub = false;
        return true;
    }
};
