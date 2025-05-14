#include <bits/stdc++.h>
#include <fstream>
using namespace std;

class SymbolInfo {
private:
    string name;
    string type;
    SymbolInfo* next;

public:
    SymbolInfo(string name, string type) : name(name), type(type), next(nullptr) {}
    ~SymbolInfo() { next = nullptr; }

    string getName() const { return name; }
    string getType() const { return type; }
    void setName(const string& n) { name = n; }
    void setType(const string& t) { type = t; }
    SymbolInfo* getNext() const { return next; }
    void setNext(SymbolInfo* n) { next = n; }

    string additionalInfo;

    void setAdditionalInfo(const string& info) { additionalInfo = info; }
    string getAdditionalInfo() const { return additionalInfo; }
};

class ScopeTable {
private:
    SymbolInfo** buckets;
    unsigned int num_buckets;
    ScopeTable* parentScope;
    int id;

    unsigned int SDBMHash(const string& str) {
        unsigned int hash = 0;
        for (char c : str) {
            hash = (c + (hash << 6) + (hash << 16) - hash) % num_buckets;
        }
        return hash;
    }

public:
    ScopeTable(int n, int id, ScopeTable* parent = nullptr) : num_buckets(n), id(id), parentScope(parent) {
        buckets = new SymbolInfo*[num_buckets]();
        for (unsigned int i = 0; i < num_buckets; i++) {
            buckets[i] = nullptr;
        }
    }

    ~ScopeTable() {
        for (unsigned int i = 0; i < num_buckets; i++) {
            SymbolInfo* current = buckets[i];
            while (current) {
                SymbolInfo* temp = current;
                current = current->getNext();
                delete temp;
            }
        }
        delete[] buckets;
    }

    pair<bool, pair<int, int>> insert(const string& name, const string& type, const string& additionalInfo = "") {
        unsigned int index = SDBMHash(name);
        SymbolInfo* current = buckets[index];
        SymbolInfo* prev = nullptr;
        int pos = 1;

        while (current) {
            if (current->getName() == name) {
                return {false, {index + 1, pos}}; // Symbol already exists
            }
            prev = current;
            current = current->getNext();
            pos++;
        }

        SymbolInfo* newSymbol = new SymbolInfo(name, type);
        if (!additionalInfo.empty()) {
            newSymbol->setAdditionalInfo(additionalInfo);
        }

        if (prev) {
            prev->setNext(newSymbol);
        } else {
            buckets[index] = newSymbol;
        }
        return {true, {index + 1, pos}};
    }

    pair<SymbolInfo*, pair<int, int>> lookup(const string& name) {
        unsigned int index = SDBMHash(name);
        SymbolInfo* current = buckets[index];
        int pos = 1;
        while (current) {
            if (current->getName() == name) {
                return {current, {index + 1, pos}};
            }
            current = current->getNext();
            pos++;
        }
        return {nullptr, {index + 1, 0}};
    }

    pair<bool, pair<int, int>> remove(const string& name) {
        unsigned int index = SDBMHash(name);
        SymbolInfo* current = buckets[index];
        SymbolInfo* prev = nullptr;
        int pos = 1;

        while (current) {
            if (current->getName() == name) {
                if (prev) {
                    prev->setNext(current->getNext());
                } else {
                    buckets[index] = current->getNext();
                }
                delete current;
                return {true, {index + 1, pos}};
            }
            prev = current;
            current = current->getNext();
            pos++;
        }
        return {false, {index + 1, 0}};
    }

    void print(ofstream& out, int indent = 0) {
        for (int i = 0; i < indent; i++) out << "\t"; // Apply indentation based on level
        out << "ScopeTable# " << id << endl;
        for (unsigned int i = 0; i < num_buckets; i++) {
            for (int j = 0; j < indent; j++) out << "\t"; // Apply indentation to bucket lines
            out << (i + 1) << "--> ";
            SymbolInfo* current = buckets[i];
            while (current) {
                out << "<" << current->getName() << "," << current->getType();
                if (!current->getAdditionalInfo().empty()) {
                    out << "," << current->getAdditionalInfo();
                }
                out << "> ";
                current = current->getNext();
            }
            out << endl;
        }
    }

    ScopeTable* getParentScope() const { return parentScope; }
    int getId() const { return id; }

    unsigned int getBucketIndex(const string& name) {
        return SDBMHash(name);
    }
};

class SymbolTable {
private:
    ScopeTable* currentScope;
    int scopeCount;
    int num_buckets;

public:
    SymbolTable(int n) : num_buckets(n), scopeCount(0) {
        enterScope();
    }

    ~SymbolTable() {
        while (currentScope) {
            ScopeTable* temp = currentScope;
            currentScope = currentScope->getParentScope();
            delete temp;
        }
    }

    void enterScope() {
        ScopeTable* newScope = new ScopeTable(num_buckets, ++scopeCount, currentScope);
        currentScope = newScope;
    }

    bool exitScope(ofstream& out) {
        if (!currentScope || scopeCount == 1) {
            return false; // Cannot exit root scope
        }
        ScopeTable* temp = currentScope;
        currentScope = currentScope->getParentScope();
        out << "\tScopeTable# " << temp->getId() << " removed" << endl;
        delete temp;
        return true;
    }

    void quit(ofstream& out) {
        vector<int> idsToRemove;
        ScopeTable* scope = currentScope;
        while (scope && scope->getId() != 1) {
            idsToRemove.push_back(scope->getId());
            scope = scope->getParentScope();
        }
        for (int id : idsToRemove) {
            ScopeTable* temp = currentScope;
            currentScope = currentScope->getParentScope();
            out << "\tScopeTable# " << id << " removed" << endl;
            delete temp;
        }
        if (currentScope) {
            out << "\tScopeTable# 1 removed" << endl;
            delete currentScope;
            currentScope = nullptr;
        }
    }

    bool insert(const string& name, const string& type, const string& additionalInfo, ofstream& out) {
        if (!currentScope) return false;
        auto result = currentScope->insert(name, type, additionalInfo);
        if (result.first) {
            out << "\tInserted in ScopeTable# " << currentScope->getId() << " at position " << result.second.first << ", " << result.second.second << endl;
        } else {
            out << "\t'" << name << "' already exists in the current ScopeTable" << endl;
        }
        return result.first;
    }

    bool remove(const string& name, ofstream& out) {
        if (!currentScope) return false;
        auto result = currentScope->remove(name);
        if (result.first) {
            out << "\tDeleted '" << name << "' from ScopeTable# " << currentScope->getId() << " at position " << result.second.first << ", " << result.second.second << endl;
        } else {
            out << "\tNot found in the current ScopeTable" << endl;
        }
        return result.first;
    }

    SymbolInfo* lookup(const string& name, ofstream& out, int& scopeId, pair<int, int>& pos) {
        ScopeTable* scope = currentScope;
        while (scope) {
            auto result = scope->lookup(name);
            if (result.first) {
                scopeId = scope->getId();
                pos = result.second;
                return result.first;
            }
            scope = scope->getParentScope();
        }
        scopeId = currentScope->getId();
        pos = {0, 0};
        return nullptr;
    }

    unsigned int getBucketIndexForSymbol(const string& name) {
        ScopeTable* scope = currentScope;
        while (scope) {
            auto result = scope->lookup(name);
            if (result.first) {
                return scope->getBucketIndex(name);
            }
            scope = scope->getParentScope();
        }
        return 0; // Not found
    }

    void printCurrentScope(ofstream& out) {
        if (currentScope) {
            currentScope->print(out, 1); // Start with indent=1 to add leading tab
        }
    }

    void printAllScopes(ofstream& out) {
        ScopeTable* scope = currentScope;
        int indent = 1; // Start with 1 to add a leading tab for the top-level scope
        while (scope) {
            scope->print(out, indent);
            scope = scope->getParentScope();
            indent++;
        }
    }

    int getCurrentScopeId() const {
        return currentScope ? currentScope->getId() : 0;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << endl;
        return 1;
    }

    ifstream in(argv[1]);
    ofstream out(argv[2]);

    if (!in.is_open() || !out.is_open()) {
        cerr << "Error opening files" << endl;
        return 1;
    }

    int num_buckets;
    in >> num_buckets;
    SymbolTable symbolTable(num_buckets);
    out << "\tScopeTable# 1 created" << endl;

    string line;
    int cmdCount = 0;
    getline(in, line); // Consume newline

    while (getline(in, line)) {
        cmdCount++;
        istringstream iss(line);
        char cmd;
        iss >> cmd;

        out << "Cmd " << cmdCount << ": " << line << endl;

        if (cmd == 'I') {
            string name, type;
            iss >> name >> type;
            string additionalInfo;

            if (type == "FUNCTION") {
                string returnType, argType;
                iss >> returnType;
                vector<string> args;
                while (iss >> argType) {
                    args.push_back(argType);
                }
                additionalInfo = returnType + "<==";
                if (!args.empty()) {
                    additionalInfo += "(";
                    for (size_t i = 0; i < args.size(); i++) {
                        additionalInfo += args[i];
                        if (i < args.size() - 1) additionalInfo += ",";
                    }
                    additionalInfo += ")";
                }
            } else if (type == "STRUCT" || type == "UNION") {
                string fieldType, fieldName;
                vector<pair<string, string>> fields;
                while (iss >> fieldType >> fieldName) {
                    fields.emplace_back(fieldType, fieldName);
                }
                if (!fields.empty()) {
                    additionalInfo = "{";
                    for (size_t i = 0; i < fields.size(); i++) {
                        additionalInfo += "(" + fields[i].first + "," + fields[i].second + ")";
                        if (i < fields.size() - 1) additionalInfo += ",";
                    }
                    additionalInfo += "}";
                }
            }

            symbolTable.insert(name, type, additionalInfo, out);
        } else if (cmd == 'L') {
            vector<string> names;
            string name;
            while (iss >> name) {
                names.push_back(name);
            }
            if (names.size() != 1) {
                out << "\tNumber of parameters mismatch for the command L" << endl;
                continue;
            }
            int scopeId;
            pair<int, int> pos;
            SymbolInfo* symbol = symbolTable.lookup(names[0], out, scopeId, pos);
            if (symbol) {
                out << "\t'" << names[0] << "' found in ScopeTable# " << scopeId << " at position " << pos.first << ", " << pos.second << endl;
            } else {
                out << "\t'" << names[0] << "' not found in any of the ScopeTables" << endl;
            }
        } else if (cmd == 'D') {
            string name;
            iss >> name;
            if (!name.empty()) {
                symbolTable.remove(name, out);
            } else {
                out << "\tNumber of parameters mismatch for the command D" << endl;
            }
        } else if (cmd == 'P') {
            char scope;
            iss >> scope;
            if (scope == 'C') {
                symbolTable.printCurrentScope(out);
            } else if (scope == 'A') {
                symbolTable.printAllScopes(out);
            }
        } else if (cmd == 'S') {
            symbolTable.enterScope();
            out << "\tScopeTable# " << symbolTable.getCurrentScopeId() << " created" << endl;
        } else if (cmd == 'E') {
            if (symbolTable.exitScope(out)) {
                // Message is handled in exitScope
            }
        } else if (cmd == 'Q') {
            symbolTable.quit(out);
            break;
        }
    }

    in.close();
    out.close();
    return 0;
}