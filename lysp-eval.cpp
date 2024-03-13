#include <iostream>
#include <string>
#include <map>
#include <fstream>
using namespace std;

struct TreeNode;
struct ListNode;

struct ListNode {
    ListNode *next;
    TreeNode *data;
};

struct TreeNode {
    ListNode *children;
    char type; // 'v' var, 'f' float, 's' string, etc.
    string value;
};

TreeNode *tree_new() {
    TreeNode *head = new TreeNode;
    head->children = NULL;
    return head;
}

ListNode *list_new(TreeNode *data) {
    ListNode *head = new ListNode;
    head->data = data;
    head->next = NULL;
    return head;
}

void list_insert_front(ListNode **head, TreeNode *data) {
    if(*head == NULL) {
        *head = list_new(data);
    } else {
        ListNode *ln = list_new(data);
        ln->next = *head;
        *head = ln;
    }
}

void list_insert_end(ListNode **head, TreeNode *data) {
    if(*head == NULL) {
        *head = list_new(data);
    } else {
        ListNode *cur = *head;
        while(cur->next != NULL) {
            cur = cur->next;
        }
        ListNode *ln = list_new(data);
        cur->next = ln;
    }
}

int parse_string(string s, int start_pos, TreeNode **head) {
    for(int i = start_pos; i < s.length(); i++) {
        // consume spaces
        while(i < s.length() && (s[i] == ' ' || s[i] == '\n')) {
            i++;
        }
        //cout << "i = " << i << " s[i] = " << s[i] << endl;
        if(s[i] == '(') {
            if(*head == NULL) { // first call
                *head = tree_new();
                i = parse_string(s, i+1, head);
            } else { // add child otherwise
                TreeNode *new_child = tree_new();
                list_insert_end(&((*head)->children), new_child);
                i = parse_string(s, i+1, &new_child);
            }
        } else if(s[i] == ')') {
            return i;
        } else if(s[i] == '"') {
            string str = "";
            i++;
            while(i < s.length() && s[i] != '"') {
                str += s[i];
                i++;
            }
            cout << "STR = " << str << endl;
            TreeNode *new_child = tree_new();
            new_child->value = str;
            new_child->type = 's';
            list_insert_end(&((*head)->children), new_child);
        } else {
            string symbol = "";
            while(i < s.length() && s[i] != ' ' && s[i] != ')' && s[i] != '(') {
                symbol += s[i];
                i++;
            }
            i--;
            TreeNode *new_child = tree_new();
            new_child->children = NULL;
            new_child->value = symbol;
            if(symbol[0] >= '0' && symbol[0] <= '9') {
                new_child->type = 'f';
            } else {
                new_child->type = 'v';
            }
            if(*head == NULL) { // first call
                *head = new_child;
            } else {
                list_insert_end(&((*head)->children), new_child);
            }
        }
    }
    return s.length();
}

void print_tree(TreeNode *head, int indent) {
    if(head == NULL) {
        return;
    }
    ListNode *child = head->children;
    while(child != NULL) {
        for(int i = 0; i <= indent; i++) {
            cout << "-";
        }
        cout << child->data->value << " " << child->data->type << endl;
        print_tree(child->data, indent+1);
        child = child->next;
    }
}

void print_tree_sexpr(TreeNode *head) {
    if(head == NULL) {
        return;
    }
    if(head->children == NULL) {
        if(head->type == 's') { cout << "\""; }
        cout << head->value << "[" << head->type << "]";
        if(head->type == 's') { cout << "\""; }
    } else {
        cout << "(";
        ListNode *child = head->children;
        while(child != NULL) {
            print_tree_sexpr(child->data);
            if(child->next != NULL) { cout << " "; }
            child = child->next;
        }
        cout << ")";
    }
}

TreeNode *eval_tree(TreeNode *head, map<string, string> env) {
    if(head->children == NULL) {
        if(head->type == 'v') {
            // look in env for that variable's value
            if(env.find(head->value) != env.end()) {
                head->value = env[head->value];
            }
        }
        return head;
    } else {
        ListNode *child = head->children;
        TreeNode *op_tn = eval_tree(child->data, env);
        child = child->next;
        if(op_tn->value == "if") {
            TreeNode *child_question = eval_tree(child->data, env);
            child = child->next;
            TreeNode *child_yes = eval_tree(child->data, env);
            child = child->next;
            TreeNode *child_no = eval_tree(child->data, env);
            if(stof(child_question->value) < 0.0) { // negative means false
                return child_no;
            } else {
                return child_yes;
            }
        } else if(op_tn->value == "let") {
            TreeNode *var_value_tn = child->data;
            ListNode *var_value_child = var_value_tn->children;
            TreeNode *var_tn = var_value_child->data;
            var_value_child = var_value_child->next;
            TreeNode *value_tn = var_value_child->data;
            string var = var_tn->value;
            string value = value_tn->value;
            env[var] = value;
            child = child->next;
            return eval_tree(child->data, env);
        } else if(op_tn->value == "input") {
            // get user input
            string input;
            cout << "Input (numeric): ";
            getline(cin, input);
            TreeNode *value_tn = tree_new();
            value_tn->value = input;
            value_tn->type = 'f';
            return value_tn;
        } else {
            TreeNode *child_tn = eval_tree(child->data, env);
            float result_val = stof(child_tn->value);
            child = child->next;
            while(child != NULL) {
                child_tn = eval_tree(child->data, env);
                if(op_tn->value == "+") {
                    result_val += stof(child_tn->value);
                } else if(op_tn->value == "-") {
                    result_val -= stof(child_tn->value);
                }
                child = child->next;
            }
            TreeNode *result_tn = new TreeNode;
            result_tn->value = to_string(result_val);
            result_tn->children = NULL;
            result_tn->type = 'f';
            return result_tn;
        }
    }
}

int main(int argc, const char**argv) {
    ifstream f(argv[1]);
    string alltext;
    string line;
    while(getline(f, line)) {
        alltext += line;
    }
    cout << "Input: " << alltext << endl;
    TreeNode *head = NULL;
    parse_string(alltext, 0, &head);
    cout << (long)head << endl;
    print_tree(head, 0);
    print_tree_sexpr(head);
    cout << endl;
    cout << "Result: ";
    map<string, string> env;
    TreeNode *result = eval_tree(head, env);
    print_tree(result, 0);
    print_tree_sexpr(result);
    cout << endl;

    return 0;
}
    

