
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <ctime>
#include <limits>
#include <algorithm>
#include <cctype>

using namespace std;

// ==================== UTILITY FUNCTIONS ====================

string getCurrentDateTime() {
    time_t now = time(nullptr);
    string result = ctime(&now);

    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    return result;
}

void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

int readInt(const string& prompt) {
    int value;

    while (true) {
        cout << prompt;

        if (cin >> value) {
            clearInput();
            return value;
        }

        cout << "Invalid input! Enter a whole number.\n";
        clearInput();
    }
}

double readAmount(const string& prompt) {
    double amount;

    while (true) {
        cout << prompt;

        if (cin >> amount && amount >= 0 &&
            amount <= 1000000000000.0) {
            clearInput();
            return amount;
        }

        cout << "Invalid amount! Try again.\n";
        clearInput();
    }
}

string readText(const string& prompt) {
    string value;

    while (true) {
        cout << prompt;
        getline(cin, value);

        if (!value.empty() &&
            value.find('|') == string::npos) {
            return value;
        }

        cout << "Input cannot be empty or contain '|'.\n";
    }
}

string readPIN(const string& prompt) {
    string pin;

    while (true) {
        cout << prompt;
        getline(cin, pin);

        if (pin.length() == 4 &&
            all_of(pin.begin(), pin.end(),
                   [](unsigned char c) {
                       return isdigit(c);
                   })) {
            return pin;
        }

        cout << "PIN must contain exactly 4 digits.\n";
    }
}

void heading(const string& title) {
    cout << "\n========================================\n";
    cout << "       " << title << "\n";
    cout << "========================================\n";
}

// ==================== TRANSACTION CLASS ====================

class Transaction {
private:
    int accountNumber;
    string description;
    double amount;
    string dateTime;

public:
    Transaction(int accNo, string desc, double amt,
                string date)
        : accountNumber(accNo),
          description(desc),
          amount(amt),
          dateTime(date) {}

    int getAccountNumber() const {
        return accountNumber;
    }

    void display() const {
        cout << left << setw(25) << description
             << " Rs. " << right << fixed
             << setprecision(2) << amount
             << "\n   " << dateTime << "\n";
    }

    void save(ofstream& file) const {
        file << accountNumber << '|'
             << description << '|'
             << fixed << setprecision(2) << amount << '|'
             << dateTime << '\n';
    }

    static bool load(const string& line, Transaction& result);
};

// ==================== ABSTRACT ACCOUNT CLASS ====================

class Account {
private:
    int accountNumber;
    string holderName;
    string pin;
    double balance;

protected:
    Account(int number, string name, string secret,
            double initialBalance)
        : accountNumber(number),
          holderName(name),
          pin(secret),
          balance(initialBalance) {}

public:
    virtual ~Account() = default;

    // Pure virtual function: makes Account abstract.
    virtual string getAccountType() const = 0;

    // Polymorphic interest calculation.
    virtual double calculateInterest() const {
        return 0.0;
    }

    int getAccountNumber() const {
        return accountNumber;
    }

    string getHolderName() const {
        return holderName;
    }

    double getBalance() const {
        return balance;
    }

    bool verifyPIN(const string& enteredPIN) const {
        return pin == enteredPIN;
    }

    bool deposit(double amount) {
        if (amount <= 0 ||
            balance + amount > 1000000000000.0) {
            return false;
        }

        balance += amount;
        return true;
    }

    bool withdraw(double amount) {
        if (amount <= 0 || amount > balance) {
            return false;
        }

        balance -= amount;
        return true;
    }

    virtual void displayDetails() const {
        cout << "\nAccount Number : " << accountNumber
             << "\nHolder Name    : " << holderName
             << "\nAccount Type   : " << getAccountType()
             << "\nBalance        : Rs. "
             << fixed << setprecision(2) << balance
             << "\nInterest       : Rs. "
             << calculateInterest() << '\n';
    }

    // Bank can access private data for file persistence.
    friend class Bank;
};

// ==================== SAVINGS ACCOUNT ====================

class SavingsAccount : public Account {
public:
    SavingsAccount(int number, string name, string pin,
                   double balance)
        : Account(number, name, pin, balance) {}

    string getAccountType() const override {
        return "Savings";
    }

    double calculateInterest() const override {
        // Illustrative annual interest rate of 4%.
        return getBalance() * 0.04;
    }
};

// ==================== CURRENT ACCOUNT ====================

class CurrentAccount : public Account {
public:
    CurrentAccount(int number, string name, string pin,
                   double balance)
        : Account(number, name, pin, balance) {}

    string getAccountType() const override {
        return "Current";
    }

    double calculateInterest() const override {
        return 0.0;
    }
};

// ==================== BANK CLASS ====================

class Bank {
private:
    vector<unique_ptr<Account>> accounts;
    vector<Transaction> transactions;

    int nextAccountNumber = 1001;

    const string ACCOUNT_FILE = "accounts.txt";
    const string TRANSACTION_FILE = "transactions.txt";

    Account* findAccount(int number) {
        for (auto& account : accounts) {
            if (account->getAccountNumber() == number) {
                return account.get();
            }
        }

        return nullptr;
    }

    void recordTransaction(int accountNumber,
                           const string& description,
                           double amount) {
        transactions.emplace_back(
            accountNumber, description, amount,
            getCurrentDateTime()
        );

        saveTransactions();
    }

    void saveAccounts() const {
        ofstream file(ACCOUNT_FILE);

        if (!file) {
            cout << "Warning: Could not save accounts.\n";
            return;
        }

        for (const auto& account : accounts) {
            file << account->getAccountNumber() << '|'
                 << account->holderName << '|'
                 << account->pin << '|'
                 << fixed << setprecision(2)
                 << account->balance << '|'
                 << account->getAccountType() << '\n';
        }
    }

    void saveTransactions() const {
        ofstream file(TRANSACTION_FILE);

        if (!file) {
            cout << "Warning: Could not save transactions.\n";
            return;
        }

        for (const auto& transaction : transactions) {
            transaction.save(file);
        }
    }

    void loadAccounts() {
        ifstream file(ACCOUNT_FILE);
        string line;

        while (getline(file, line)) {
            stringstream ss(line);
            string numberText, name, pin, balanceText, type;

            if (!getline(ss, numberText, '|') ||
                !getline(ss, name, '|') ||
                !getline(ss, pin, '|') ||
                !getline(ss, balanceText, '|') ||
                !getline(ss, type)) {
                continue;
            }

            try {
                int number = stoi(numberText);
                double balance = stod(balanceText);

                if (number <= 0 || balance < 0 ||
                    balance > 1000000000000.0 ||
                    pin.length() != 4 ||
                    !all_of(pin.begin(), pin.end(),
                        [](unsigned char c) {
                            return isdigit(c);
                        })) {
                    continue;
                }

                if (findAccount(number) != nullptr) {
                    continue;
                }

                if (type == "Savings") {
                    accounts.push_back(
                        make_unique<SavingsAccount>(
                            number, name, pin, balance
                        )
                    );
                } else if (type == "Current") {
                    accounts.push_back(
                        make_unique<CurrentAccount>(
                            number, name, pin, balance
                        )
                    );
                } else {
                    continue;
                }

                nextAccountNumber =
                    max(nextAccountNumber, number + 1);

            } catch (const exception&) {
                cout << "Skipping an invalid account record.\n";
            }
        }
    }

    void loadTransactions() {
        ifstream file(TRANSACTION_FILE);
        string line;

        while (getline(file, line)) {
            stringstream ss(line);
            string numberText, description, amountText, date;

            if (!getline(ss, numberText, '|') ||
                !getline(ss, description, '|') ||
                !getline(ss, amountText, '|') ||
                !getline(ss, date)) {
                continue;
            }

            try {
                int number = stoi(numberText);
                double amount = stod(amountText);

                transactions.emplace_back(
                    number, description, amount, date
                );
            } catch (const exception&) {
                // Ignore malformed transaction records.
            }
        }
    }

    void createAccount() {
        heading("CREATE BANK ACCOUNT");

        string name = readText("Enter full name: ");
        string pin = readPIN("Set a 4-digit PIN: ");
        double initialBalance =
            readAmount("Opening deposit (Rs.): ");

        cout << "\n1. Savings Account"
             << "\n2. Current Account\n";

        int type = readInt("Choose account type: ");

        if (type != 1 && type != 2) {
            cout << "Invalid account type.\n";
            return;
        }

        if (initialBalance > 1000000000000.0) {
            cout << "Opening balance exceeds the limit.\n";
            return;
        }

        int number = nextAccountNumber++;

        if (type == 1) {
            accounts.push_back(
                make_unique<SavingsAccount>(
                    number, name, pin, initialBalance
                )
            );
        } else {
            accounts.push_back(
                make_unique<CurrentAccount>(
                    number, name, pin, initialBalance
                )
            );
        }

        recordTransaction(
            number, "Opening Deposit", initialBalance
        );

        saveAccounts();

        cout << "\nAccount created successfully!\n"
             << "Your Account Number: " << number
             << "\nPlease remember your account number and PIN.\n";
    }

    Account* login() {
        heading("CUSTOMER LOGIN");

        int number = readInt("Account number: ");
        string pin = readPIN("Enter your 4-digit PIN: ");

        Account* account = findAccount(number);

        if (account == nullptr || !account->verifyPIN(pin)) {
            cout << "Invalid account number or PIN.\n";
            return nullptr;
        }

        cout << "\nWelcome, "
             << account->getHolderName() << "!\n";

        return account;
    }

    void depositMoney(Account* account) {
        heading("DEPOSIT MONEY");

        double amount = readAmount("Amount to deposit: Rs. ");

        if (!account->deposit(amount)) {
            cout << "Deposit failed. Check amount and balance limit.\n";
            return;
        }

        recordTransaction(
            account->getAccountNumber(), "Deposit", amount
        );
        saveAccounts();

        cout << "Deposit successful!\n"
             << "New balance: Rs. "
             << fixed << setprecision(2)
             << account->getBalance() << '\n';
    }

    void withdrawMoney(Account* account) {
        heading("WITHDRAW MONEY");

        double amount = readAmount("Amount to withdraw: Rs. ");

        if (!account->withdraw(amount)) {
            cout << "Withdrawal failed: insufficient balance "
                 << "or invalid amount.\n";
            return;
        }

        recordTransaction(
            account->getAccountNumber(), "Withdrawal", amount
        );
        saveAccounts();

        cout << "Withdrawal successful!\n"
             << "Remaining balance: Rs. "
             << fixed << setprecision(2)
             << account->getBalance() << '\n';
    }

    void transferMoney(Account* sender) {
        heading("TRANSFER MONEY");

        int receiverNumber =
            readInt("Receiver account number: ");

        if (receiverNumber == sender->getAccountNumber()) {
            cout << "You cannot transfer money to yourself.\n";
            return;
        }

        Account* receiver = findAccount(receiverNumber);

        if (receiver == nullptr) {
            cout << "Receiver account not found.\n";
            return;
        }

        double amount = readAmount("Transfer amount: Rs. ");

        // Validate both sides before modifying either account.
        if (amount <= 0 || amount > sender->getBalance() ||
            receiver->getBalance() + amount > 1000000000000.0) {
            cout << "Transfer failed. Check balance and amount.\n";
            return;
        }

        if (!sender->withdraw(amount)) {
            cout << "Transfer failed.\n";
            return;
        }

        if (!receiver->deposit(amount)) {
            // Roll back if the receiver deposit fails.
            sender->deposit(amount);
            cout << "Transfer failed. No money was transferred.\n";
            return;
        }

        recordTransaction(
            sender->getAccountNumber(),
            "Transfer to " + to_string(receiverNumber),
            amount
        );

        recordTransaction(
            receiver->getAccountNumber(),
            "Transfer from " +
                to_string(sender->getAccountNumber()),
            amount
        );

        saveAccounts();

        cout << "Transfer successful!\n"
             << "Receiver: " << receiver->getHolderName()
             << "\nAmount: Rs. " << fixed << setprecision(2)
             << amount
             << "\nRemaining balance: Rs. "
             << sender->getBalance() << '\n';
    }

    void showTransactionHistory(Account* account) const {
        heading("TRANSACTION HISTORY");

        bool found = false;

        for (const auto& transaction : transactions) {
            if (transaction.getAccountNumber() ==
                account->getAccountNumber()) {
                transaction.display();
                found = true;
            }
        }

        if (!found) {
            cout << "No transactions found.\n";
        }
    }

    void customerMenu() {
        Account* account = login();

        if (account == nullptr) {
            return;
        }

        int choice;

        do {
            heading("CUSTOMER DASHBOARD");

            cout << "1. View Account Details\n"
                 << "2. Check Balance\n"
                 << "3. Deposit Money\n"
                 << "4. Withdraw Money\n"
                 << "5. Transfer Money\n"
                 << "6. Transaction History\n"
                 << "0. Logout\n";

            choice = readInt("Enter choice: ");

            switch (choice) {
                case 1:
                    account->displayDetails();
                    break;

                case 2:
                    cout << "Available balance: Rs. "
                         << fixed << setprecision(2)
                         << account->getBalance() << '\n';
                    break;

                case 3:
                    depositMoney(account);
                    break;

                case 4:
                    withdrawMoney(account);
                    break;

                case 5:
                    transferMoney(account);
                    break;

                case 6:
                    showTransactionHistory(account);
                    break;

                case 0:
                    cout << "Logged out successfully.\n";
                    break;

                default:
                    cout << "Invalid choice. Try again.\n";
            }

        } while (choice != 0);
    }

    void showAllAccounts() const {
        heading("ALL BANK ACCOUNTS");

        if (accounts.empty()) {
            cout << "No accounts registered.\n";
            return;
        }

        cout << left
             << setw(12) << "Account No."
             << setw(24) << "Holder Name"
             << setw(15) << "Type"
             << "Balance (Rs.)\n";

        cout << string(68, '-') << '\n';

        for (const auto& account : accounts) {
            cout << left
                 << setw(12) << account->getAccountNumber()
                 << setw(24) << account->getHolderName()
                 << setw(15) << account->getAccountType()
                 << fixed << setprecision(2)
                 << account->getBalance() << '\n';
        }
    }

    void searchAccount() const {
        heading("SEARCH ACCOUNT");

        int number = readInt("Enter account number: ");

        for (const auto& account : accounts) {
            if (account->getAccountNumber() == number) {
                account->displayDetails();
                return;
            }
        }

        cout << "Account not found.\n";
    }

    void showBankSummary() const {
        heading("BANK SUMMARY");

        double totalBalance = 0;
        int savingsCount = 0;
        int currentCount = 0;

        for (const auto& account : accounts) {
            totalBalance += account->getBalance();

            if (account->getAccountType() == "Savings") {
                savingsCount++;
            } else {
                currentCount++;
            }
        }

        cout << "Total Accounts   : " << accounts.size()
             << "\nSavings Accounts : " << savingsCount
             << "\nCurrent Accounts : " << currentCount
             << "\nTotal Deposits   : Rs. "
             << fixed << setprecision(2)
             << totalBalance << '\n';
    }

    void adminMenu() {
        heading("ADMIN LOGIN");

        string username = readText("Username: ");
        string password = readText("Password: ");

        // Demo credentials. Do not use this design in production.
        if (username != "admin" || password != "admin123") {
            cout << "Incorrect admin credentials.\n";
            return;
        }

        cout << "Admin login successful!\n";

        int choice;

        do {
            heading("ADMIN DASHBOARD");

            cout << "1. View All Accounts\n"
                 << "2. Search Account\n"
                 << "3. Bank Summary\n"
                 << "0. Logout\n";

            choice = readInt("Enter choice: ");

            switch (choice) {
                case 1:
                    showAllAccounts();
                    break;

                case 2:
                    searchAccount();
                    break;

                case 3:
                    showBankSummary();
                    break;

                case 0:
                    cout << "Admin logged out.\n";
                    break;

                default:
                    cout << "Invalid choice.\n";
            }

        } while (choice != 0);
    }

public:
    Bank() {
        loadAccounts();
        loadTransactions();
    }

    void run() {
        int choice;

        do {
            heading("WELCOME TO ABC BANK");

            cout << "1. Create Account\n"
                 << "2. Customer Login\n"
                 << "3. Admin Login\n"
                 << "0. Exit\n";

            choice = readInt("Enter your choice: ");

            switch (choice) {
                case 1:
                    createAccount();
                    break;

                case 2:
                    customerMenu();
                    break;

                case 3:
                    adminMenu();
                    break;

                case 0:
                    saveAccounts();
                    saveTransactions();
                    cout << "\nThank you for using ABC Bank!\n";
                    break;

                default:
                    cout << "Invalid choice. Try again.\n";
            }

        } while (choice != 0);
    }
};

// ==================== MAIN FUNCTION ====================

int main() {
    cout << fixed << setprecision(2);

    Bank bank;
    bank.run();

    return 0;
}
