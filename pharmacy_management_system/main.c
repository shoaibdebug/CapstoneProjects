#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>

/* ---------------------------- File names ---------------------------- */
// Let's set up our file names here. Think of these as the garages where we park all our data.
#define USER_FILE       "users.dat"
#define MEDICINE_FILE   "medicines.dat"
#define CUSTOMER_FILE   "customers.dat"
#define SALES_FILE      "sales.dat"
#define MED_TEMP_FILE   "medtemp.dat"

// If a medicine drops below this number, the system will flag it as low stock.
#define LOW_STOCK_THRESHOLD 10

/* ------------------------------ Structs ------------------------------ */

// Just a simple way to keep track of dates without overcomplicating things.
typedef struct {
    int dd, mm, yy;
} Date;

// This defines who is logging in. The 'role' field is super important because
// it tells us if they get Admin powers or just Pharmacist access.
typedef struct {
    int  userID;
    char username[20];
    char password[20];
    char role[15];
} User;

// Everything we need to know about a single medicine in our inventory.
typedef struct {
    int   medID;
    char  name[30];
    char  company[30];
    float price;
    int   quantity;
    Date  expiry;
} Medicine;

// Keeping track of our buyers.
typedef struct {
    int  custID;
    char name[30];
    char phone[15];
} Customer;

// This acts like a digital receipt for every sale we make.
typedef struct {
    int   billNo;
    int   custID;
    int   medID;
    int   quantity;
    float totalAmount;
    Date  salesDate;
} Sales;

/* --------------------------- Global state ---------------------------- */
// We keep this global so the system always remembers who is currently logged in.
User currentUser;

/* ----------------------------- Prototypes ----------------------------- */
// UI & little helper functions
void gotoxy(int x, int y);
void delay(unsigned int mseconds);
void pressEnterToContinue(void);
void getCurrentDate(Date *d);

// Login and user setup stuff
void initializeUsers(void);
void login(void);
void adminMenu(void);
void pharmacistMenu(void);
void logoutProgram(void);

// The core inventory functions (Adding, updating, deleting medicines)
int  getNextMedicineID(void);
void addMedicine(void);
void updateMedicine(void);
void deleteMedicine(void);
void viewMedicineList(void);
void printMedicineTableHeader(int row);
void printMedicineRow(Medicine m, int row);

// Search and sort logic
void searchMedicineMenu(void);
void searchByID(void);
void searchByName(void);
void searchByCompany(void);
int  countMedicines(void);
void sortMedicineMenu(void);

// Checking for expired or low stock items
void viewLowStock(void);
void viewExpiredMedicines(void);

// Customer handling
int  getNextCustomerID(void);
int  checkCustomerExists(int id);
void addCustomer(void);
void viewCustomers(void);
void manageCustomerMenu(void);

// The money makers: Selling and billing
int  getNextBillNumber(void);
int  findMedicineByID(int id, Medicine *out);
int  updateMedicineQuantity(int id, int newQty);
void sellMedicine(void);
void generateBill(Sales s, Medicine m);

// Reporting features to see how business is doing
void dailySalesReport(void);
void monthlySalesReport(void);
void viewSalesHistory(void);

/* ------------------------------------------------------------------- */
/*  Console helpers                                                     */
/* ------------------------------------------------------------------- */

// This is a neat little trick to move the cursor exactly where we want it on the screen.
// Makes the UI look much cleaner than just printing line by line.
void gotoxy(int x, int y)
{
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// Just pauses the program for a split second. Great for creating a smooth experience.
void delay(unsigned int mseconds)
{
    // [BUG FIX]: Replaced the CPU-heavy while loop with Windows' built-in Sleep() function.
    Sleep(mseconds);
}

// A simple prompt so the screen doesn't clear before the user has a chance to read it.
void pressEnterToContinue(void)
{
    gotoxy(2, 22);
    printf("Press ENTER to continue...");
    while (getch() != 13)
        ;
}

// Grabs the actual time from your computer and formats it into our Date struct.
void getCurrentDate(Date *d)
{
    time_t t = time(NULL);
    struct tm *info = localtime(&t);
    d->dd = info->tm_mday;
    d->mm = info->tm_mon + 1;
    d->yy = info->tm_year + 1900;
}

/* ------------------------------------------------------------------- */
/*  Startup / Authentication                                            */
/* ------------------------------------------------------------------- */

// When you run this for the very first time, this function sets up a default Admin
// and Pharmacist account so you aren't locked out of your own system.
void initializeUsers(void)
{
    FILE *fp = fopen(USER_FILE, "rb");
    if (fp != NULL)
    {
        // If the file already has data, we just skip this part. No need to overwrite.
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fclose(fp);
        if (size > 0)
            return;
    }

    User admin, pharmacist;

    // Hardcoding the first admin account.
    admin.userID = 1;
    strcpy(admin.username, "admin");
    strcpy(admin.password, "admin123");
    strcpy(admin.role, "Admin");

    // Hardcoding the first pharmacist account.
    pharmacist.userID = 2;
    strcpy(pharmacist.username, "pharma");
    strcpy(pharmacist.password, "pharma123");
    strcpy(pharmacist.role, "Pharmacist");

    fp = fopen(USER_FILE, "wb");
    if (fp == NULL)
    {
        printf("Error: could not create user file.\n");
        return;
    }
    fwrite(&admin, sizeof(User), 1, fp);
    fwrite(&pharmacist, sizeof(User), 1, fp);
    fclose(fp);
}

// This handles the login stuff. It even hides the password with asterisks as you type,
// which adds a nice professional touch.
void login(void)
{
    char uname[20];
    char pass[20];
    char ch;
    int  i, found = 0;
    FILE *fp;
    User u;

    system("cls");
    gotoxy(20, 3);
    printf("========================================");
    gotoxy(20, 4);
    printf("       PHARMACY MANAGEMENT SYSTEM");
    gotoxy(20, 5);
    printf("========================================");

    gotoxy(18, 8);
    printf("Username : ");
    scanf("%19s", uname);

    gotoxy(18, 10);
    printf("Password : ");

    // This while loop is what actually converts your keystrokes into asterisks.
    i = 0;
    while ((ch = getch()) != 13) // 13 is the ENTER key
    {
        if (ch == 8) // Handles the backspace key so you can fix typos
        {
            if (i > 0)
            {
                i--;
                printf("\b \b");
            }
        }
        else if (i < 19)
        {
            pass[i] = ch;
            i++;
            putch('*');
        }
    }
    pass[i] = '\0';

    fp = fopen(USER_FILE, "rb");
    if (fp == NULL)
    {
        gotoxy(18, 13);
        printf("User file not found. Cannot continue.");
        getch();
        exit(1);
    }

    // Checking if what you typed matches any record in our file.
    while (fread(&u, sizeof(User), 1, fp) == 1)
    {
        if (strcmp(u.username, uname) == 0 && strcmp(u.password, pass) == 0)
        {
            currentUser = u;
            found = 1;
            break;
        }
    }
    fclose(fp);

    if (found)
    {
        gotoxy(18, 13);
        printf("Login successful. Welcome, %s.", currentUser.role);
        delay(800);

        // Send them to the right menu depending on their job title.
        if (strcmp(currentUser.role, "Admin") == 0)
            adminMenu();
        else
            pharmacistMenu();
    }
    else
    {
        gotoxy(18, 13);
        printf("\aInvalid username or password.");
        getch();
        login(); // If they mess up, just run the login screen again.
    }
}

// A nice little goodbye message before the program closes out completely.
void logoutProgram(void)
{
    system("cls");
    gotoxy(15, 10);
    printf("Thank you for using the Pharmacy Management System.");
    gotoxy(15, 11);
    printf("Exiting...");
    delay(1200);
    exit(0);
}

/* ------------------------------------------------------------------- */
/*  Menus                                                                */
/* ------------------------------------------------------------------- */

// The main hub for Admins. They get access to everything, including reports and deleting records.
void adminMenu(void)
{
    int choice;

    // This infinite loop keeps throwing the menu back on screen until they hit "Logout".
    while (1)
    {
        system("cls");
        gotoxy(10, 2);
        printf("================================================================");
        gotoxy(10, 3);
        printf("                   ADMINISTRATOR MAIN MENU");
        gotoxy(10, 4);
        printf("================================================================");
        gotoxy(10, 6);  printf("1.  Add Medicine");
        gotoxy(10, 7);  printf("2.  Update Medicine");
        gotoxy(10, 8);  printf("3.  Delete Medicine");
        gotoxy(10, 9);  printf("4.  View Medicine List");
        gotoxy(10, 10); printf("5.  Search Medicine");
        gotoxy(10, 11); printf("6.  Sort Medicine");
        gotoxy(10, 12); printf("7.  Manage Customer Information");
        gotoxy(10, 13); printf("8.  View Low Stock Medicines");
        gotoxy(10, 14); printf("9.  View Expired Medicines");
        gotoxy(10, 15); printf("10. Generate Daily Sales Report");
        gotoxy(10, 16); printf("11. Generate Monthly Sales Report");
        gotoxy(10, 17); printf("12. View Sales History");
        gotoxy(10, 18); printf("13. Logout");
        gotoxy(10, 20); printf("----------------------------------------------------------------");
        gotoxy(10, 21); printf("Enter your choice: ");

        // [BUG FIX]: Clearing the input buffer. If someone types a letter (like 'A') instead of a number,
        // this stops the menu from going totally crazy and looping infinitely. Just saving the program from crashing, you know?
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            choice = -1; // Forces it into the default switch case.
        }

        switch (choice)
        {
            case 1:  addMedicine();        break;
            case 2:  updateMedicine();     break;
            case 3:  deleteMedicine();     break;
            case 4:  viewMedicineList();   break;
            case 5:  searchMedicineMenu(); break;
            case 6:  sortMedicineMenu();   break;
            case 7:  manageCustomerMenu(); break;
            case 8:  viewLowStock();       break;
            case 9:  viewExpiredMedicines(); break;
            case 10: dailySalesReport();   break;
            case 11: monthlySalesReport(); break;
            case 12: viewSalesHistory();   break;
            case 13: logoutProgram();      return;
            default:
                gotoxy(10, 23);
                printf("\aInvalid choice. Please try again.");
                getch();
        }
    }
}

// The hub for Pharmacists. They can sell, search, and check stock, but they can't mess with reports.
void pharmacistMenu(void)
{
    int choice;

    while (1)
    {
        system("cls");
        gotoxy(10, 2);
        printf("================================================================");
        gotoxy(10, 3);
        printf("                     PHARMACIST MAIN MENU");
        gotoxy(10, 4);
        printf("================================================================");
        gotoxy(10, 6);  printf("1. Search Medicine");
        gotoxy(10, 7);  printf("2. View Medicine List");
        gotoxy(10, 8);  printf("3. Sell Medicine");
        gotoxy(10, 9);  printf("4. View Low Stock Medicines");
        gotoxy(10, 10); printf("5. View Expired Medicines");
        gotoxy(10, 11); printf("6. View Customer Information");
        gotoxy(10, 12); printf("7. Logout");
        gotoxy(10, 14); printf("----------------------------------------------------------------");
        gotoxy(10, 15); printf("Enter your choice: ");

        // [BUG FIX]: Catching bad inputs here too. We really don't want the system to hang
        // if a busy pharmacist accidentally hits a letter key!
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            choice = -1;
        }

        switch (choice)
        {
            case 1: searchMedicineMenu(); break;
            case 2: viewMedicineList();   break;
            case 3: sellMedicine();       break;
            case 4: viewLowStock();       break;
            case 5: viewExpiredMedicines(); break;
            case 6: viewCustomers();      break;
            case 7: logoutProgram();      return;
            default:
                gotoxy(10, 17);
                printf("\aInvalid choice. Please try again.");
                getch();
        }
    }
}

/* ------------------------------------------------------------------- */
/*  Medicine Management (FR-02, FR-09)                                  */
/* ------------------------------------------------------------------- */

// Scans the file to find the highest medicine ID and just adds 1 to it.
// Prevents us from ever having duplicate IDs.
int getNextMedicineID(void)
{
    FILE *fp = fopen(MEDICINE_FILE, "rb");
    Medicine m;
    int maxID = 0;

    if (fp != NULL)
    {
        while (fread(&m, sizeof(Medicine), 1, fp) == 1)
            if (m.medID > maxID)
                maxID = m.medID;
        fclose(fp);
    }
    return maxID + 1;
}

// Where we actually put new stock into the system.
void addMedicine(void)
{
    Medicine m;
    FILE *fp;
    char choice;

    do
    {
        system("cls");
        gotoxy(10, 2);
        printf("======================= ADD MEDICINE =======================");

        m.medID = getNextMedicineID();
        gotoxy(10, 4);
        printf("Medicine ID      : %d", m.medID);

        gotoxy(10, 5);
        printf("Medicine Name    : ");
        // This specific scanf format lets us capture names with spaces, like "Napa Extra"
        scanf(" %29[^\n]", m.name);

        gotoxy(10, 6);
        printf("Company Name     : ");
        scanf(" %29[^\n]", m.company);

        gotoxy(10, 7);
        printf("Price            : ");
        // A tiny loop to make sure nobody accidentally enters a negative price.
        do { scanf("%f", &m.price); } while(m.price < 0);

        gotoxy(10, 8);
        printf("Quantity         : ");
        do { scanf("%d", &m.quantity); } while(m.quantity < 0);

        gotoxy(10, 9);
        printf("Expiry (dd mm yyyy): ");
        // Simple logic to keep the dates realistic.
        do { scanf("%d %d %d", &m.expiry.dd, &m.expiry.mm, &m.expiry.yy); }
        while(m.expiry.dd < 1 || m.expiry.dd > 31 || m.expiry.mm < 1 || m.expiry.mm > 12 || m.expiry.yy < 2000);

        // We use 'ab' mode so it just tacks the new info onto the end of the file.
        fp = fopen(MEDICINE_FILE, "ab");
        if (fp == NULL)
        {
            gotoxy(10, 12);
            printf("\aError: could not open medicine file.");
        }
        else
        {
            fwrite(&m, sizeof(Medicine), 1, fp);
            fclose(fp);
            gotoxy(10, 12);
            printf("Medicine record added successfully.");
        }

        gotoxy(10, 14);
        printf("Add another medicine? (Y/N): ");
        choice = getch();
        printf("%c", choice);

    } while (choice == 'y' || choice == 'Y');

    pressEnterToContinue();
}

// When you need to fix a typo or update the price of an existing medicine.
void updateMedicine(void)
{
    int id, found = 0;
    Medicine m;
    FILE *fp;

    system("cls");
    gotoxy(10, 2);
    printf("====================== UPDATE MEDICINE ======================");
    gotoxy(10, 4);
    printf("Enter Medicine ID to update: ");
    scanf("%d", &id);

    // Using 'rb+' so we can read the file, find the exact record, and overwrite just that part.
    fp = fopen(MEDICINE_FILE, "rb+");
    if (fp == NULL)
    {
        gotoxy(10, 6);
        printf("No medicine records found.");
        pressEnterToContinue();
        return;
    }

    while (fread(&m, sizeof(Medicine), 1, fp) == 1)
    {
        if (m.medID == id)
        {
            found = 1;
            gotoxy(10, 6);
            printf("Current Name     : %s", m.name);

            gotoxy(10, 7);
            printf("New Name         : ");
            scanf(" %29[^\n]", m.name);

            gotoxy(10, 8);
            printf("New Company Name : ");
            scanf(" %29[^\n]", m.company);

            gotoxy(10, 9);
            printf("New Price        : ");
            do { scanf("%f", &m.price); } while(m.price < 0);

            gotoxy(10, 10);
            printf("New Quantity     : ");
            do { scanf("%d", &m.quantity); } while(m.quantity < 0);

            gotoxy(10, 11);
            printf("New Expiry (dd mm yyyy): ");
            do { scanf("%d %d %d", &m.expiry.dd, &m.expiry.mm, &m.expiry.yy); }
            while(m.expiry.dd < 1 || m.expiry.dd > 31 || m.expiry.mm < 1 || m.expiry.mm > 12 || m.expiry.yy < 2000);

            // This is the magic trick. We roll the file pointer back exactly one record's length
            // so we can overwrite the old data with the new stuff.
            fseek(fp, -(long)sizeof(Medicine), SEEK_CUR);
            fwrite(&m, sizeof(Medicine), 1, fp);
            break;
        }
    }
    fclose(fp);

    gotoxy(10, 13);
    if (found)
        printf("Medicine record updated successfully.");
    else
        printf("\aMedicine record not found.");

    pressEnterToContinue();
}

// In C, you can't just delete a line from a binary file easily.
// So, we copy everything EXCEPT the record we want to delete into a new temporary file,
// then we just swap the files. It is like moving to a new house and leaving the junk behind.
void deleteMedicine(void)
{
    int id, found = 0;
    Medicine m;
    FILE *fp, *ft;

    system("cls");
    gotoxy(10, 2);
    printf("====================== DELETE MEDICINE ======================");
    gotoxy(10, 4);
    printf("Enter Medicine ID to delete: ");
    scanf("%d", &id);

    fp = fopen(MEDICINE_FILE, "rb");
    ft = fopen(MED_TEMP_FILE, "wb");

    if (fp == NULL || ft == NULL)
    {
        gotoxy(10, 6);
        printf("\aError accessing medicine records.");
        if (fp != NULL) fclose(fp);
        if (ft != NULL) fclose(ft);
        pressEnterToContinue();
        return;
    }

    while (fread(&m, sizeof(Medicine), 1, fp) == 1)
    {
        if (m.medID == id)
            found = 1; // Found the guy we want to delete! Don't copy him.
        else
            fwrite(&m, sizeof(Medicine), 1, ft); // Copy everyone else.
    }
    fclose(fp);
    fclose(ft);

    // Delete the old file, rename the new one. Clean and simple.
    remove(MEDICINE_FILE);
    rename(MED_TEMP_FILE, MEDICINE_FILE);

    gotoxy(10, 6);
    if (found)
        printf("Medicine record deleted successfully.");
    else
        printf("\aMedicine record not found.");

    pressEnterToContinue();
}

// Just printing the top row for our tables so things look aligned.
void printMedicineTableHeader(int row)
{
    gotoxy(2, row);
    printf("ID     NAME            COMPANY         PRICE     QTY    EXPIRY");
    gotoxy(2, row + 1);
    printf("------------------------------------------------------------------");
}

// Formats the actual data neatly under the headers.
void printMedicineRow(Medicine m, int row)
{
    gotoxy(2, row);  printf("%-6d", m.medID);
    gotoxy(9, row);  printf("%-15s", m.name);
    gotoxy(25, row); printf("%-15s", m.company);
    gotoxy(41, row); printf("%-9.2f", m.price);
    gotoxy(51, row); printf("%-6d", m.quantity);
    gotoxy(58, row); printf("%02d-%02d-%04d", m.expiry.dd, m.expiry.mm, m.expiry.yy);
}

// Reads the whole inventory file from top to bottom and spits it out on screen.
void viewMedicineList(void)
{
    Medicine m;
    FILE *fp;
    int row = 4;

    system("cls");
    printMedicineTableHeader(2);

    fp = fopen(MEDICINE_FILE, "rb");
    if (fp == NULL)
    {
        gotoxy(2, row);
        printf("No medicine records found.");
    }
    else
    {
        while (fread(&m, sizeof(Medicine), 1, fp) == 1)
        {
            printMedicineRow(m, row);
            row++;
        }
        fclose(fp);
    }

    pressEnterToContinue();
}

/* ------------------------------------------------------------------- */
/*  Search Medicine (FR-05)                                             */
/* ------------------------------------------------------------------- */

// A mini-menu just to figure out how the user wants to search.
void searchMedicineMenu(void)
{
    int choice;

    while (1)
    {
        system("cls");
        gotoxy(10, 3);
        printf("====================== SEARCH MEDICINE ======================");
        gotoxy(10, 5); printf("1. Search by Medicine ID");
        gotoxy(10, 6); printf("2. Search by Medicine Name");
        gotoxy(10, 7); printf("3. Search by Company Name");
        gotoxy(10, 8); printf("4. Back to Menu");
        gotoxy(10, 10); printf("Enter your choice: ");

        // [BUG FIX]: Keeping the search menu safe from accidental character inputs.
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            choice = -1;
        }

        switch (choice)
        {
            case 1: searchByID();      break;
            case 2: searchByName();    break;
            case 3: searchByCompany(); break;
            case 4: return;
            default:
                gotoxy(10, 12);
                printf("\aInvalid choice.");
                getch();
        }
    }
}

// Finding a specific medicine by its unique ID number.
void searchByID(void)
{
    int id, found = 0, row = 6;
    Medicine m;
    FILE *fp;

    system("cls");
    gotoxy(10, 3);
    printf("Enter Medicine ID: ");
    scanf("%d", &id);

    printMedicineTableHeader(4);

    fp = fopen(MEDICINE_FILE, "rb");
    if (fp != NULL)
    {
        while (fread(&m, sizeof(Medicine), 1, fp) == 1)
        {
            if (m.medID == id)
            {
                printMedicineRow(m, row);
                row++;
                found = 1;
            }
        }
        fclose(fp);
    }

    if (!found)
    {
        gotoxy(2, row);
        printf("\aNo record found.");
    }

    pressEnterToContinue();
}

// Searching by name. Just a heads up, this requires an exact match right now.
void searchByName(void)
{
    char name[30];
    int found = 0, row = 6;
    Medicine m;
    FILE *fp;

    system("cls");
    gotoxy(10, 3);
    printf("Enter Medicine Name: ");
    scanf(" %29[^\n]", name);

    printMedicineTableHeader(4);

    fp = fopen(MEDICINE_FILE, "rb");
    if (fp != NULL)
    {
        while (fread(&m, sizeof(Medicine), 1, fp) == 1)
        {
            // [BUG FIX]: Replaced strcmp with _stricmp to make the search case-insensitive.
            if (_stricmp(m.name, name) == 0)
            {
                printMedicineRow(m, row);
                row++;
                found = 1;
            }
        }
        fclose(fp);
    }

    if (!found)
    {
        gotoxy(2, row);
        printf("\aNo record found.");
    }

    pressEnterToContinue();
}

// Showing all medicines made by a specific company.
void searchByCompany(void)
{
    char company[30];
    int found = 0, row = 6;
    Medicine m;
    FILE *fp;

    system("cls");
    gotoxy(10, 3);
    printf("Enter Company Name: ");
    scanf(" %29[^\n]", company);

    printMedicineTableHeader(4);

    fp = fopen(MEDICINE_FILE, "rb");
    if (fp != NULL)
    {
        while (fread(&m, sizeof(Medicine), 1, fp) == 1)
        {
            // [BUG FIX]: Replaced strcmp with _stricmp to make the search case-insensitive.
            if (_stricmp(m.company, company) == 0)
            {
                printMedicineRow(m, row);
                row++;
                found = 1;
            }
        }
        fclose(fp);
    }

    if (!found)
    {
        gotoxy(2, row);
        printf("\aNo record found.");
    }

    pressEnterToContinue();
}

/* ------------------------------------------------------------------- */
/*  Sorting (FR-07)                                                      */
/* ------------------------------------------------------------------- */

// Quickly counts how many records we have so we know how much memory to allocate later.
int countMedicines(void)
{
    FILE *fp = fopen(MEDICINE_FILE, "rb");
    Medicine m;
    int count = 0;

    if (fp != NULL)
    {
        while (fread(&m, sizeof(Medicine), 1, fp) == 1)
            count++;
        fclose(fp);
    }
    return count;
}

// We pull all the records out of the file, put them in a temporary array,
// sort them using Bubble Sort, print them, and then free the memory.
void sortMedicineMenu(void)
{
    int choice, n, i, j, k;
    Medicine *list;
    Medicine temp;
    FILE *fp;

    while (1)
    {
        system("cls");
        gotoxy(10, 3);
        printf("====================== SORT MEDICINE ======================");
        gotoxy(10, 5); printf("1. Sort by Price (Low to High)");
        gotoxy(10, 6); printf("2. Sort by Expiry Date (Earliest First)");
        gotoxy(10, 7); printf("3. Back to Menu");
        gotoxy(10, 9); printf("Enter your choice: ");

        // [BUG FIX]: Standard buffer flush. Ensures the user is forced back to the menu gracefully if they mess up.
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            choice = -1;
        }

        if (choice == 3)
        {
            return;
        }
        if (choice != 1 && choice != 2)
        {
            gotoxy(10, 11);
            printf("\aInvalid choice.");
            getch();
            continue;
        }

        n = countMedicines();
        if (n == 0)
        {
            gotoxy(10, 11);
            printf("No medicine records found.");
            pressEnterToContinue();
            return;
        }

        // Grabbing the exact amount of memory we need based on the record count.
        list = (Medicine *)malloc(n * sizeof(Medicine));
        if (list == NULL)
        {
            gotoxy(10, 11);
            printf("\aMemory allocation failed.");
            pressEnterToContinue();
            return;
        }

        // Load everything into the array we just created.
        fp = fopen(MEDICINE_FILE, "rb");
        i = 0;
        while (fread(&list[i], sizeof(Medicine), 1, fp) == 1)
            i++;
        fclose(fp);

        // Good old Bubble Sort. Not the fastest, but perfectly fine for a capstone project.
        for (j = 0; j < n - 1; j++)
        {
            for (k = 0; k < n - 1 - j; k++)
            {
                int swapNeeded = 0;

                if (choice == 1)
                {
                    // If we are sorting by price...
                    if (list[k].price > list[k + 1].price)
                        swapNeeded = 1;
                }
                else
                {
                    // If we are sorting by date. I combined the year, month, and day into a single
                    // long number (like 20260520) to make comparing them super easy.
                    long v1 = list[k].expiry.yy * 10000L + list[k].expiry.mm * 100 + list[k].expiry.dd;
                    long v2 = list[k + 1].expiry.yy * 10000L + list[k + 1].expiry.mm * 100 + list[k + 1].expiry.dd;
                    if (v1 > v2)
                        swapNeeded = 1;
                }

                // Swap them around if they are in the wrong order.
                if (swapNeeded)
                {
                    temp = list[k];
                    list[k] = list[k + 1];
                    list[k + 1] = temp;
                }
            }
        }

        system("cls");
        printMedicineTableHeader(2);
        for (i = 0; i < n; i++)
            printMedicineRow(list[i], 4 + i);

        // ALWAYS free your memory when you're done, folks! Prevents memory leaks.
        free(list);
        pressEnterToContinue();
    }
}

/* ------------------------------------------------------------------- */
/*  Inventory Alerts (FR-04)                                            */
/* ------------------------------------------------------------------- */

// Filters the list and shows us what is running out soon based on that threshold we set earlier.
void viewLowStock(void)
{
    Medicine m;
    FILE *fp;
    int row = 5, found = 0;

    system("cls");
    gotoxy(2, 2);
    printf("============= LOW STOCK MEDICINES (Quantity < %d) =============", LOW_STOCK_THRESHOLD);
    printMedicineTableHeader(3);

    fp = fopen(MEDICINE_FILE, "rb");
    if (fp != NULL)
    {
        while (fread(&m, sizeof(Medicine), 1, fp) == 1)
        {
            if (m.quantity < LOW_STOCK_THRESHOLD)
            {
                printMedicineRow(m, row);
                row++;
                found = 1;
            }
        }
        fclose(fp);
    }

    if (!found)
    {
        gotoxy(2, row);
        printf("No low stock medicines found.");
    }

    pressEnterToContinue();
}

// Checks every medicine against today's actual date to see what belongs in the trash.
void viewExpiredMedicines(void)
{
    Medicine m;
    FILE *fp;
    Date today;
    long todayVal, expVal;
    int row = 5, found = 0;

    // Get today's date and turn it into that easily comparable number format.
    getCurrentDate(&today);
    todayVal = today.yy * 10000L + today.mm * 100 + today.dd;

    system("cls");
    gotoxy(2, 2);
    printf("========================= EXPIRED MEDICINES =========================");
    printMedicineTableHeader(3);

    fp = fopen(MEDICINE_FILE, "rb");
    if (fp != NULL)
    {
        while (fread(&m, sizeof(Medicine), 1, fp) == 1)
        {
            expVal = m.expiry.yy * 10000L + m.expiry.mm * 100 + m.expiry.dd;
            if (expVal < todayVal)
            {
                printMedicineRow(m, row);
                row++;
                found = 1;
            }
        }
        fclose(fp);
    }

    if (!found)
    {
        gotoxy(2, row);
        printf("No expired medicines found.");
    }

    pressEnterToContinue();
}

/* ------------------------------------------------------------------- */
/*  Customer Management (FR-03)                                         */
/* ------------------------------------------------------------------- */

// Generates the next ID for a new customer without us having to guess.
int getNextCustomerID(void)
{
    FILE *fp = fopen(CUSTOMER_FILE, "rb");
    Customer c;
    int maxID = 0;

    if (fp != NULL)
    {
        while (fread(&c, sizeof(Customer), 1, fp) == 1)
            if (c.custID > maxID)
                maxID = c.custID;
        fclose(fp);
    }
    return maxID + 1;
}

// Just a quick check to see if this customer already exists in our system.
int checkCustomerExists(int id)
{
    FILE *fp = fopen(CUSTOMER_FILE, "rb");
    Customer c;

    if (fp == NULL)
        return 0;

    while (fread(&c, sizeof(Customer), 1, fp) == 1)
    {
        if (c.custID == id)
        {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

// Adds a new customer to our file. Nothing too crazy here.
void addCustomer(void)
{
    Customer c;
    FILE *fp;

    system("cls");
    gotoxy(10, 3);
    printf("====================== ADD CUSTOMER ======================");

    c.custID = getNextCustomerID();
    gotoxy(10, 5);
    printf("Customer ID   : %d", c.custID);

    gotoxy(10, 6);
    printf("Customer Name : ");
    scanf(" %29[^\n]", c.name);

    gotoxy(10, 7);
    printf("Phone Number  : ");
    scanf("%14s", c.phone);

    fp = fopen(CUSTOMER_FILE, "ab");
    if (fp == NULL)
    {
        gotoxy(10, 9);
        printf("\aError saving customer record.");
    }
    else
    {
        fwrite(&c, sizeof(Customer), 1, fp);
        fclose(fp);
        gotoxy(10, 9);
        printf("Customer added successfully.");
    }

    pressEnterToContinue();
}

// Spits out a list of everyone who has bought from us.
void viewCustomers(void)
{
    Customer c;
    FILE *fp;
    int row = 4;

    system("cls");
    gotoxy(2, 2);
    printf("ID     NAME                 PHONE");
    gotoxy(2, 3);
    printf("--------------------------------------------------");

    fp = fopen(CUSTOMER_FILE, "rb");
    if (fp == NULL)
    {
        gotoxy(2, row);
        printf("No customer records found.");
    }
    else
    {
        while (fread(&c, sizeof(Customer), 1, fp) == 1)
        {
            gotoxy(2, row);  printf("%-6d", c.custID);
            gotoxy(9, row);  printf("%-20s", c.name);
            gotoxy(30, row); printf("%-15s", c.phone);
            row++;
        }
        fclose(fp);
    }

    pressEnterToContinue();
}

// A little sub-menu to keep the customer tasks organized.
void manageCustomerMenu(void)
{
    int choice;

    while (1)
    {
        system("cls");
        gotoxy(10, 3);
        printf("=================== CUSTOMER MANAGEMENT ===================");
        gotoxy(10, 5); printf("1. Add Customer");
        gotoxy(10, 6); printf("2. View Customer List");
        gotoxy(10, 7); printf("3. Back to Menu");
        gotoxy(10, 9); printf("Enter your choice: ");

        // [BUG FIX]: Same deal here. A nice little safety net for our customer management menu.
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            choice = -1;
        }

        switch (choice)
        {
            case 1: addCustomer();  break;
            case 2: viewCustomers(); break;
            case 3: return;
            default:
                gotoxy(10, 11);
                printf("\aInvalid choice.");
                getch();
        }
    }
}

/* ------------------------------------------------------------------- */
/*  Sales Management (FR-06)                                            */
/* ------------------------------------------------------------------- */

// Keeps the invoice numbers rolling sequentially.
int getNextBillNumber(void)
{
    FILE *fp = fopen(SALES_FILE, "rb");
    Sales s;
    int maxBill = 0;

    if (fp != NULL)
    {
        while (fread(&s, sizeof(Sales), 1, fp) == 1)
            if (s.billNo > maxBill)
                maxBill = s.billNo;
        fclose(fp);
    }
    return maxBill + 1;
}

// Grabs a medicine's details based on its ID so we can use its price and check stock.
int findMedicineByID(int id, Medicine *out)
{
    FILE *fp = fopen(MEDICINE_FILE, "rb");
    Medicine m;

    if (fp == NULL)
        return 0;

    while (fread(&m, sizeof(Medicine), 1, fp) == 1)
    {
        if (m.medID == id)
        {
            *out = m; // Copying it to our output variable
            fclose(fp);
            return 1; // Success!
        }
    }
    fclose(fp);
    return 0; // Couldn't find it.
}

// This deducts the quantity from our stock *after* a sale happens.
int updateMedicineQuantity(int id, int newQty)
{
    FILE *fp = fopen(MEDICINE_FILE, "rb+");
    Medicine m;

    if (fp == NULL)
        return 0;

    while (fread(&m, sizeof(Medicine), 1, fp) == 1)
    {
        if (m.medID == id)
        {
            m.quantity = newQty;
            fseek(fp, -(long)sizeof(Medicine), SEEK_CUR);
            fwrite(&m, sizeof(Medicine), 1, fp);
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

// This is where the magic happens. Linking the customer, checking stock, deducting items, and billing.
void sellMedicine(void)
{
    int custID, medID, qty;
    Medicine m;
    Sales s;
    Customer c;
    FILE *fp;

    system("cls");
    gotoxy(10, 2);
    printf("======================= SELL MEDICINE =======================");

    gotoxy(10, 4);
    printf("Enter Customer ID (0 if new customer): ");
    scanf("%d", &custID);

    // If it's a new face, we register them right here so we don't have to back out to another menu.
    if (custID == 0 || !checkCustomerExists(custID))
    {
        gotoxy(10, 6);
        printf("Registering new customer...");

        c.custID = getNextCustomerID();
        gotoxy(10, 7);
        printf("Customer ID   : %d", c.custID);

        gotoxy(10, 8);
        printf("Customer Name : ");
        scanf(" %29[^\n]", c.name);

        gotoxy(10, 9);
        printf("Phone Number  : ");
        scanf("%14s", c.phone);

        fp = fopen(CUSTOMER_FILE, "ab");
        if (fp != NULL)
        {
            fwrite(&c, sizeof(Customer), 1, fp);
            fclose(fp);
        }
        custID = c.custID; // Boom, we have our customer ID.
    }

    gotoxy(10, 11);
    printf("Enter Medicine ID: ");
    scanf("%d", &medID);

    if (!findMedicineByID(medID, &m))
    {
        gotoxy(10, 13);
        printf("\aMedicine not found.");
        pressEnterToContinue();
        return;
    }

    // Showing what we've got before we try to sell it.
    gotoxy(10, 13);
    printf("Medicine Name    : %s", m.name);
    gotoxy(10, 14);
    printf("Available Stock  : %d", m.quantity);
    gotoxy(10, 15);
    printf("Price Per Unit   : %.2f", m.price);

    gotoxy(10, 17);
    printf("Enter Quantity to Sell: ");
    scanf("%d", &qty);

    // Can't sell what we don't have, right?
    if (qty <= 0 || qty > m.quantity)
    {
        gotoxy(10, 19);
        printf("\aInvalid quantity or insufficient stock.");
        pressEnterToContinue();
        return;
    }

    // Setting up the receipt
    s.billNo = getNextBillNumber();
    s.custID = custID;
    s.medID = medID;
    s.quantity = qty;
    s.totalAmount = qty * m.price;
    getCurrentDate(&s.salesDate);

    // We make sure the stock actually drops before we finalize the sale in the records.
    if (updateMedicineQuantity(medID, m.quantity - qty))
    {
        fp = fopen(SALES_FILE, "ab");
        if (fp != NULL)
        {
            fwrite(&s, sizeof(Sales), 1, fp);
            fclose(fp);
        }
        generateBill(s, m);
    }
    else
    {
        gotoxy(10, 19);
        printf("\aError updating stock! Sale cancelled.");
    }

    pressEnterToContinue();
}

// Clears the screen and prints out a nice-looking invoice for the customer.
void generateBill(Sales s, Medicine m)
{
    system("cls");
    gotoxy(10, 2);
    printf("======================= SALES INVOICE =======================");
    gotoxy(10, 4);  printf("Bill Number    : %d", s.billNo);
    gotoxy(10, 5);  printf("Date           : %02d-%02d-%04d", s.salesDate.dd, s.salesDate.mm, s.salesDate.yy);
    gotoxy(10, 6);  printf("Customer ID    : %d", s.custID);
    gotoxy(10, 8);  printf("Medicine       : %s", m.name);
    gotoxy(10, 9);  printf("Quantity       : %d", s.quantity);
    gotoxy(10, 10); printf("Price/Unit     : %.2f", m.price);
    gotoxy(10, 11); printf("-----------------------------------");
    gotoxy(10, 12); printf("Total Amount   : %.2f", s.totalAmount);
    gotoxy(10, 14); printf("Thank you for your purchase!");
}

/* ------------------------------------------------------------------- */
/*  Reports (FR-08)                                                      */
/* ------------------------------------------------------------------- */

// Compares the sales records with today's date so we know exactly what we made today.
void dailySalesReport(void)
{
    Sales s;
    FILE *fp;
    Date today;
    int row, found = 0;
    float grandTotal = 0.0f;

    getCurrentDate(&today);

    system("cls");
    gotoxy(2, 2);
    printf("============= DAILY SALES REPORT (%02d-%02d-%04d) =============",
           today.dd, today.mm, today.yy);
    gotoxy(2, 4);
    printf("BILL NO   CUST ID   MED ID   QTY     TOTAL");
    row = 5;

    fp = fopen(SALES_FILE, "rb");
    if (fp != NULL)
    {
        while (fread(&s, sizeof(Sales), 1, fp) == 1)
        {
            if (s.salesDate.dd == today.dd && s.salesDate.mm == today.mm && s.salesDate.yy == today.yy)
            {
                gotoxy(2, row);  printf("%-10d", s.billNo);
                gotoxy(12, row); printf("%-10d", s.custID);
                gotoxy(22, row); printf("%-9d", s.medID);
                gotoxy(31, row); printf("%-8d", s.quantity);
                gotoxy(39, row); printf("%.2f", s.totalAmount);
                grandTotal += s.totalAmount; // Tallying up the money!
                row++;
                found = 1;
            }
        }
        fclose(fp);
    }

    if (!found)
    {
        gotoxy(2, row);
        printf("No sales recorded for today.");
        row++;
    }

    gotoxy(2, row + 1);
    printf("Total Sales Amount: %.2f", grandTotal);

    pressEnterToContinue();
}

// Similar to the daily report, but you punch in a specific month and year to check.
void monthlySalesReport(void)
{
    Sales s;
    FILE *fp;
    int month, year, row, found = 0;
    float grandTotal = 0.0f;

    system("cls");
    gotoxy(2, 2);
    printf("========================= MONTHLY SALES REPORT =========================");
    gotoxy(2, 4);
    printf("Enter Month (1-12): ");
    scanf("%d", &month);
    gotoxy(2, 5);
    printf("Enter Year        : ");
    scanf("%d", &year);

    gotoxy(2, 7);
    printf("BILL NO   CUST ID   MED ID   QTY     TOTAL");
    row = 8;

    fp = fopen(SALES_FILE, "rb");
    if (fp != NULL)
    {
        while (fread(&s, sizeof(Sales), 1, fp) == 1)
        {
            if (s.salesDate.mm == month && s.salesDate.yy == year)
            {
                gotoxy(2, row);  printf("%-10d", s.billNo);
                gotoxy(12, row); printf("%-10d", s.custID);
                gotoxy(22, row); printf("%-9d", s.medID);
                gotoxy(31, row); printf("%-8d", s.quantity);
                gotoxy(39, row); printf("%.2f", s.totalAmount);
                grandTotal += s.totalAmount;
                row++;
                found = 1;
            }
        }
        fclose(fp);
    }

    if (!found)
    {
        gotoxy(2, row);
        printf("No sales recorded for %d-%d.", month, year);
        row++;
    }

    gotoxy(2, row + 1);
    printf("Total Sales Amount: %.2f", grandTotal);

    pressEnterToContinue();
}

// This just dumps out every single sale we have ever made.
void viewSalesHistory(void)
{
    Sales s;
    FILE *fp;
    int row = 4;
    float grandTotal = 0.0f;

    system("cls");
    gotoxy(2, 2);
    printf("BILL NO   CUST ID   MED ID   QTY     TOTAL         DATE");
    gotoxy(2, 3);
    printf("------------------------------------------------------------");

    fp = fopen(SALES_FILE, "rb");
    if (fp == NULL)
    {
        gotoxy(2, row);
        printf("No sales records found.");
    }
    else
    {
        while (fread(&s, sizeof(Sales), 1, fp) == 1)
        {
            gotoxy(2, row);  printf("%-10d", s.billNo);
            gotoxy(12, row); printf("%-10d", s.custID);
            gotoxy(22, row); printf("%-9d", s.medID);
            gotoxy(31, row); printf("%-8d", s.quantity);
            gotoxy(39, row); printf("%-14.2f", s.totalAmount);
            gotoxy(53, row); printf("%02d-%02d-%04d", s.salesDate.dd, s.salesDate.mm, s.salesDate.yy);
            grandTotal += s.totalAmount;
            row++;
        }
        fclose(fp);

        gotoxy(2, row + 1);
        printf("Grand Total: %.2f", grandTotal);
    }

    pressEnterToContinue();
}

/* ------------------------------------------------------------------- */
/*  Entry point                                                          */
/* ------------------------------------------------------------------- */

// Where it all begins! Kicks off the user setup and throws you straight to the login screen.
int main(void)
{
    initializeUsers();
    login();
    return 0;
}
