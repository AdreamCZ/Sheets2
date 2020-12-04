// Tabulkový editor v textovém prostředí 2.0
// Autor : Aram Denk (xdenka00) 

#define _DEFAULT_SOURCE //Umožní použití strdup a dalších i s std=c99
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>



#define BASE_ROWSTR_SIZE 10
#define BASE_COMMAND_COUNT 10
#define SELECTALL -420 //Identifikátor _ v selekci
#define DOUBLEMAXVAL 1.79769e308


typedef struct Cols{
    long length;
    char *chars;
}Col;

typedef struct Rows{
    long length;
    Col *cols;
} Row;

typedef struct Tables{
    long length;
    Row *rows;
}Table;

typedef struct CommandsS{
    int length;
    char **commands;
}Commands;

typedef struct selCols{ //struktura pro vybranou buňku
    long row;
    long col;
}selCol;

typedef struct Selections{ //Více vybraných buněk
    long long length;
    selCol *cells;
}Selection;

void help(){ //Vytiskne nápovědu
    printf("Program pro práci s tabulkami v čistě textovém prostředí\n");
    printf("Syntax spouštění : ./sps [-d DELIM] CMD_SEQUENCE FILE \n");
    printf("Argument -d specifikuje, jaké znaky lze interpretovat jako oddělovače jednotlivých buněk.\n");
    printf("Argument CMD_SEQUENCE je jeden argument obsahující sekvenci příkazů. Více příkazů tabulkového procesoru je odděleno středníkem.\n");
    printf("Argument FILE specifikuje název souboru s tabulkou.\n");
}

void delChars(char *str,int from, int length) // Z libovolné pozice řetezce smaže několik znaků
{
  if((unsigned)(length+from) <= strlen(str))
  {
    for(int i = 0; i < length ; i++){
        memmove(&str[from],&str[from+1],strlen(str)-from);
    }
  }
}


bool isDel(char *str,int pos, char *del){ //Kontroluje zda-li je znak na pozici pos řetezce str (řádek) oddělovač 
    if(strchr(del,str[pos]) != 0){
        if( pos == 0 || str[pos-1] != '\\' ){ //Pokud oddělovač není uvozen zpětným lomítkem
            long parCnt = 0; //Počet uvozovek před pozicí pos
            for(int i = 0; i < pos; i++){
                if(str[i] == '\"'){
                    parCnt++;
                }
            }
            if(parCnt % 2 != 0){ //Pokud je počet uvozovek lichý znamená to že symbol na pozici pos je uzavřen mezi uvozovkami => není oddělovač
                return false;
            }else{
                return true;
            }
        }
    }
    return false;
}

void freeTable(Table *tab){ //Uvolní pamět allokovanou pro tabulku //TODO somehow test dis
    for(int r = 0; r < tab->length;r++){
        for(int c = 0; c < tab->rows[r].length;c++){
            free(tab->rows[r].cols[c].chars);
        }
        free(tab->rows[r].cols);
    }
    free(tab->rows);
}

void printTable(Table *table, char *del, char *url){ //Zapíše tabulku do souboru
    if(table->length > 0){
        FILE *fp = fopen(url,"w");

        for(long r = 0; r < table->length;r++){
            for(long c = 0; c < table->rows[r].length; c++){ // 
                bool hasSpecial = false;
                if(strchr(table->rows[r].cols[c].chars,'\\') != NULL && strchr(table->rows[r].cols[c].chars,'"') == 0){
                    putc('\"',fp);
                    hasSpecial = true;
                }
                bool escapeLastQutation = false;
                for(long long s = 0; s < table->rows[r].cols[c].length;s++){
                    printf("%c",table->rows[r].cols[c].chars[s]);
                    if((table->rows[r].cols[c].chars[s] == '"' && (s != 0 && s != table->rows[r].cols[c].length - 1))||( table->rows[r].cols[c].chars[s] == '"' && escapeLastQutation)){ //Pokud je uvozovka vevnitř buňky, nebo pokud je na konci a předchozí uvozovka nebyla na začátku => musí být escapovaná
                        putc('\\',fp);
                        putc('"',fp);
                        escapeLastQutation = true; //Uvozovka byla uvozena zpětným lomítkem, tedy i další uvozovka v buňce musí být také uvozena zpětným lomítkem
                    }else if(table->rows[r].cols[c].chars[s] == '"'){ //Uvozovka je na začátku buňky => nemusí být escapovaná
                        putc('"',fp);
                    }else if(table->rows[r].cols[c].chars[s] != '\\'){
                        putc(table->rows[r].cols[c].chars[s],fp);
                    }else if(s > 0 &&table->rows[r].cols[c].chars[s] == '\\'&& table->rows[r].cols[c].chars[s - 1] == '\\'){ //Znak na pozici s je zpětné lomítko, ale předchozí znak byl také zpětné lomítko => jedno zpětné lomítko má být zapsáno
                        putc('\\',fp);
                    }
                }
                if(hasSpecial){
                    putc('\"',fp);
                }
                if( c < table->rows[r].length - 1){
                   putc(del[0],fp);
                }
            }
            putc('\n',fp);
        }
        fclose(fp);
    }
}

Row loadRow(char *rowStr, char *del){ // Řetězec s řádkem rozdělí na buňky a uloží do struktury
    Row row;
    row.length = 0;
    row.cols = malloc(sizeof(Col));
    int colCounter = 0;
    for(int i = 0; rowStr[i] != '\0';i++){ 
        if(isDel(rowStr,i,del)){ //Znak je delimiter -> jdu uložit buňku před ním 
            Col *np = realloc(row.cols, sizeof(Col) * (colCounter + 1));
            if(np == NULL){
                free(row.cols);
                perror("Error");
                row.length = -1;
                return row;
            }else{
                row.cols = np;
            }
            Col col;
            col.chars=malloc(sizeof(char) * i + 1);
            col.length = i;
            strncpy(col.chars,rowStr,i);
            col.chars[i] = '\0';
            delChars(rowStr,0,i+1);
            row.cols[colCounter] = col; //Vložení vytvořené buňky do řádku
            colCounter++;
            row.length++;
            i = 0;
        }
    }
    Col *np = realloc(row.cols, sizeof(Col) * (colCounter + 1)); 
    if(np == NULL){
        free(row.cols);
        perror("Error");
        row.length = -1;
        return row;
    }else{
        row.cols = np;
    }

    //Uložení poslední buňky
    row.cols = realloc(row.cols,sizeof(Col) * (colCounter + 1));
    row.cols[colCounter].chars = strdup(rowStr);
    row.cols[colCounter].length = strlen(rowStr);
    row.length++;
    
    return row;
}


void loadTable(Table *table,char *url,char *del){ // Načte tabulku ze souboru a uloží ji do struktury
    table->length = 0;
    table->rows = malloc(sizeof(Row));

    FILE *fp = fopen(url,"r");
    if(fp == NULL){ 
        perror("Error"); 
        table->length = -1;
        return; 
    }
    int counter = 1;

    char *rowStr = malloc(sizeof(char) * BASE_ROWSTR_SIZE);
    //Načítá po jednom řádku
    while(fgets(rowStr,BASE_ROWSTR_SIZE - 1,fp) != NULL){
        if(rowStr[0] == EOF || rowStr[0] == '\0'){
            table->length = -1;
            return;
        }
        int reallocCount = 2;

        //Zvětšení paměti pro tabulku o řádek
        Row *np = realloc(table->rows,counter * sizeof(Row));
        if(np == NULL){
            perror("Error");
            free(table->rows);
            table->length = -1;
            fclose(fp);
            return;
        }else{
            table->rows = np;
        }

        while(rowStr[strlen(rowStr) - 1] != '\n' &&  feof(fp)==0){ //rowStr není dost velká pro načtení celého řádku
            //Zvětšení paměti pro řádek
            char *str2 = malloc(sizeof(char) * BASE_ROWSTR_SIZE);
            fgets(str2,BASE_ROWSTR_SIZE,fp); //Přečte zbytek řádku
            char *np = realloc(rowStr, reallocCount * sizeof(char) * BASE_ROWSTR_SIZE);
            reallocCount++;
            if(np == NULL){
                free(rowStr);
                perror("Error");
                table->length = -1;
                fclose(fp);
                return;
            }else{
                rowStr = np;
                strcat(rowStr,str2);
            }
            free(str2);
        }
        for(int i = 0; rowStr[i] != '\0'; i++){ //Vymaže znaky, které by mohli později způsobovat problémy
            if(rowStr[i] == '\n' || rowStr[i] == '\r' || rowStr[i] == '\t'){
                delChars(rowStr,i,1);
                i--;
            }
        }
        Row row = loadRow(rowStr,del);  //Uloží řádek do struktury
        
        if(row.length == -1){
            table->length = -1;
            fclose(fp);
            return;
        }
        table->length++;

        table->rows[counter - 1] = row;
        counter++;
    }
    free(rowStr);
    fclose(fp);
    return;
}

void alignCols(Table *table,long colCount){ //Zajistí stejný počet sloupců ve všech řádcích, pokud je colCount větší něž počet sloupců nejdelšího řádku je nový počet sloupců v každém řádku colCount
    int maxCols = 0;
    for(int i = 0; i < table->length;i++){
        maxCols = table->rows[i].length > maxCols ? table->rows[i].length : maxCols; //Zjistí kolik sloupců má nejdelší řádek
    }
    if(colCount != 0 && colCount < maxCols){
        return;
    }else{
        maxCols = maxCols >= colCount + 1 ? maxCols : colCount + 1;
    }
    for(int r = 0; r < table->length;r++){
        if(table->rows[r].length < maxCols){
            Col *np = realloc(table->rows[r].cols, (maxCols+1) * sizeof(Col));
            if(np == NULL){
                perror("Error ");
                table->length = -1;
                return;
            }else{
                table->rows[r].cols = np;
            }
            for(long c = table->rows[r].length;c < maxCols;c++){    
                table->rows[r].cols[c].length = 0;
                table->rows[r].cols[c].chars = malloc( sizeof(char) );
                table->rows[r].cols[c].chars[0] = '\0';

            }
            table->rows[r].length = maxCols;  
        }
    }
}

Commands parseCommands(char *commsStr){ //Řetězec obsahující několik příkazů rozdělí na jednotilivé příkazy
    Commands comms;
    
    if(strstr(commsStr,".txt") != NULL){ // Řetězec není série příkazů, ale cesta k souboru
        comms.length = 0;
        return comms;
    }
    
    comms.length = 0;
    if(commsStr[0] == '\0'){ // Jako argument s příkazy bylo zadáno jen ""
        comms.length = -1; //Délka -1 značí chybu (Platí pro všechny v programu)
        return comms;
    }
    comms.commands = malloc(BASE_COMMAND_COUNT * sizeof(char *));
    bool work = true;
    int cnt = 0;
    while(work){
        int semColLoc = -1;
        
        if((cnt+1) % BASE_COMMAND_COUNT == 0){
            char **np = realloc(comms.commands, (cnt+1) * sizeof(char *));
            if(np == NULL){
                perror("Error");
                comms.length = -1;
                return comms;
            }else{
                comms.commands = np;
            }
        }
        if(strchr(commsStr,';') != NULL){
            semColLoc = strchr(commsStr,';') - commsStr;
            comms.commands[cnt] = malloc( semColLoc  * sizeof(char) + 1);
            strncpy(comms.commands[cnt],commsStr,semColLoc);
            comms.commands[cnt][semColLoc] = '\0';
            delChars(commsStr,0,semColLoc + 1);
            
        }else{
            comms.commands[cnt] = strdup(commsStr);
            commsStr[0] = '\0';
        }
        cnt++;
        comms.length++;
        if(commsStr[0] == '\0'){
            work = false;
        }
    }
    return comms;

}

//Funkce úpravu strukutry tabulky :
bool irow(Table *tab,Selection *sel){ //Vloží prázný řádek před výběr
    //Přidává řádek i naend 
    tab->length = tab->length + 1;
    Row *np = realloc(tab->rows,tab->length * sizeof(Row));
    if(np == NULL)
        return false;
    tab->rows = np;
    tab->rows[tab->length - 1].cols = NULL;

    long insertLoc = sel->cells[0].row;
    long colCount = tab->rows[sel->cells[0].row].length;
    
    for(long r = tab->length - 1; r > insertLoc; r--){// Zdvojí řádek na insertLoc a ostatní posune dolů
        tab->rows[r].cols = realloc(tab->rows[r].cols, colCount * sizeof(Col));
        tab->rows[r].length = colCount;
        for(long c = 0; c < colCount; c++){
            char *newValue = tab->rows[r - 1].cols[c].chars;
            if(r == tab->length - 1) //tab->rows[r].cols[c].chars ještě není inicializované
                tab->rows[r].cols[c].chars = malloc((strlen(newValue) + 1) * sizeof(char));
            else{
                char *np = realloc(tab->rows[r].cols[c].chars,(strlen(newValue) + 1) * sizeof(char));
                if(np == NULL) tab->length = -1;
                tab->rows[r].cols[c].chars = np;
            }
            tab->rows[r].cols[c].length = strlen(newValue);
            strcpy(tab->rows[r].cols[c].chars, newValue);
        }
    }

    for(long c = 0; c < colCount; c++){ //vloží prázdný řádek
            char *np = realloc(tab->rows[insertLoc].cols[c].chars,sizeof(char));
            if(np == NULL) tab->length = -1;
            tab->rows[insertLoc].cols[c].chars = np;
            tab->rows[insertLoc].cols[c].length = 0;
            tab->rows[insertLoc].cols[c].chars[0] = '\0';
    }

    return true;
}

bool arow(Table *tab, Selection *sel){ //arow  Vloží prázdný řádek za výběr
    tab->length = tab->length + 1;
    Row *np = realloc(tab->rows,tab->length * sizeof(Row));
    if(np == NULL)
        return false;
    tab->rows = np;

    long insertLoc = sel->cells[sel->length - 1].row + 1;
    long colCount = tab->rows[0].length;

    for(long r = tab->length - 1; r > insertLoc; r--){// Zdvojí řádek na insertLoc a ostatní posune dolů
        tab->rows[r].cols = realloc(tab->rows[r].cols, colCount  * sizeof(Col));
        tab->rows[r].length = colCount;
        for(long c = 0; c < colCount; c++){ //Kopíruje z buňky v předchozím řádku do současného řádku
            char *newValue = strdup(tab->rows[r - 1].cols[c].chars);
            if(r == tab->length - 1){ //tab->rows[r].cols[c].chars ještě není inicializované
                tab->rows[r].cols[c].chars = malloc((strlen(newValue) + 1) * sizeof(char));
            }else{
                char *np = realloc(tab->rows[r].cols[c].chars,(strlen(newValue) + 1) * sizeof(char));
                if(np == NULL) tab->length = -1;
                tab->rows[r].cols[c].chars = np;
            }
            tab->rows[r].cols[c].length = strlen(newValue);
            strcpy(tab->rows[r].cols[c].chars, newValue);
            free(newValue);
        }
    }
    if(insertLoc == tab->length - 1){ //Nový řádek má být přidán na end tabulky
        tab->rows[insertLoc].cols = malloc(colCount * sizeof(Col)); 
        tab->rows[tab->length - 1].length = colCount;
    }
    for(long c = 0; c < colCount; c++){ //vloží prázdný řádek
        if(insertLoc != tab->length - 1){
            char *np = realloc(tab->rows[insertLoc].cols[c].chars,sizeof(char));
            if(np == NULL) return false;
            tab->rows[insertLoc].cols[c].chars = np;
        }
        tab->rows[insertLoc].cols[c].chars = malloc(sizeof(char));
        tab->rows[insertLoc].cols[c].length = 0;
        tab->rows[insertLoc].cols[c].chars[0] = '\0';
    }

    return true;
}

bool drow(Table *tab, Selection *sel){ //Smaže řádky ve výběru
    long colCount = tab->rows[0].length;
    for(int i = 0; i < sel->length; i++){
        long delLoc = sel->cells[i].row;
        char *newValue;
        for(int r = delLoc; r < tab->length - 1; r++){
            for(long c = 0; c < colCount; c++){
                newValue = tab->rows[r + 1].cols[c].chars;
                char *np = realloc(tab->rows[r].cols[c].chars,(strlen(newValue) + 1) * sizeof(char));
                if(np == NULL) return false;
                tab->rows[r].cols[c].chars = np;
                tab->rows[r].cols[c].length = strlen(newValue);
                strcpy(tab->rows[r].cols[c].chars, newValue);
            }
        }
        tab->rows[tab->length - 1].length = 0;
        tab->rows[tab->length - 1].cols = NULL;
        free(tab->rows[tab->length - 1].cols);
        tab->length--;
        Row *np = realloc(tab->rows,tab->length * sizeof(Row));
        if(np == NULL)  
            return false;
        tab->rows = np;
    }
    return true;
}

bool icol(Table *tab, Selection *sel){ //vloží jeden prázdný sloupec nalevo od vybraných buněk.
    long insertLoc = sel->cells[0].col;

    for(long r = 0; r < tab->length; r++){
        Col *np = realloc( tab->rows[r].cols,(tab->rows[r].length + 1) * sizeof(Col)); //Zvětší alokovanou pamět pro řádek
        if(np == NULL)
            return false;
        tab->rows[r].cols = np;
        tab->rows[r].cols[tab->rows[r].length].chars = NULL;
        for(long c = tab->rows[r].length; c > insertLoc; c--){ //Zduplikuje sloupec na insertLoc a ostatní posune doprava
            char *np = realloc(tab->rows[r].cols[c].chars,(strlen(tab->rows[r].cols[c-1].chars) + 1)*sizeof(char));
            if(np == NULL) return false;
            tab->rows[r].cols[c].chars = np;
            strcpy(tab->rows[r].cols[c].chars,tab->rows[r].cols[c - 1].chars);
            tab->rows[r].cols[c].length = strlen(tab->rows[r].cols[c].chars);
        }
        tab->rows[r].length++;
        tab->rows[r].cols[insertLoc].length = 0; //Vymaže obsah sloupce na insertLoc
        char *ncp = realloc(tab->rows[r].cols[insertLoc].chars,sizeof(char));
        if(ncp == NULL) return false;
        tab->rows[r].cols[insertLoc].chars = ncp;
        tab->rows[r].cols[insertLoc].chars[0] = '\0'; 
    }
    return true;
}

bool acol(Table *tab, Selection *sel){ //přidá jeden prázdný sloupec napravo od vybraných buněk.
    Selection newSel;
    newSel.length = 1; //Nová selekce
    newSel.cells = malloc(sizeof(selCol));
    newSel.cells[0].col = sel->cells[sel->length - 1].col + 1; //Sloupec nového výběru je o jedna větší něž největší sloupec původního vůběru
    newSel.cells[0].row = 1;
    bool rtn = icol(tab,&newSel); //icol s novým výběrem
    free(newSel.cells);
    return rtn;
}

bool dcol(Table *tab, Selection *sel){ //odstraní vybrané sloupce
    long row;
    for(long i = sel->length - 1; i >= 0 ; i--){ //TODO Provede se mockrát např když je vybrána celá tabulka počet je dvakrát větší
        if(i == sel->length - 1) row = sel->cells[i].row; //Vyberu jeden řádek, který beru v potaz a mažu pro něj sloupce. Pokud bych toto vynechal, pak např při výběru [_,3] by nebyl smazán pouze jeden sloupec.
        if(sel->cells[i].row != row) break; //Pokud řádek není selekce není TEN vybraný, nepokračuje.
        for(long r = 0; r < tab->length; r++){
            for(int c = sel->cells[i].col; c < tab->rows[r].length; c++){//smaže sloupec a ostatní posune doleva
                // tab->rows[r].cols[c].chars = tab->rows[r].cols[c + 1].chars; ¨
                if(c != tab->rows[r].length - 1){ 
                    char *np = realloc(tab->rows[r].cols[c].chars, strlen(tab->rows[r].cols[c + 1].chars) + 1);
                    if(np == NULL) return false;
                    tab->rows[r].cols[c].chars = np;
                    strcpy(tab->rows[r].cols[c].chars,tab->rows[r].cols[c + 1].chars);
                    tab->rows[r].cols[c].length = tab->rows[r].cols[c + 1].length;
                }else{//Smaže poslední sloupec (je zdvojený)
                    free(tab->rows[r].cols[c].chars);
                    tab->rows[r].cols[c].length = 0;
                    tab->rows[r].length--;
                }
            }
            tab->rows[r].cols = realloc(tab->rows[r].cols, tab->rows[r].length * sizeof(Col));
        }
    }
    return true;
}
//Funkce pro selekci buněk :
selCol selectCell(Table *tab, long r, long c){
    selCol cell;
    if( (r < 0 && r!=SELECTALL) || (c < 0 && c != SELECTALL)){
        cell.col = -1;
        cell.row = -1;
        return cell;
    }
    if(r + 1 > tab->length){ //Selekce je mimo tabulku -> přidání řádků
        for(int i = tab->length - 1; i < r;i++){
            Selection sel;
            sel.length = 1;
            sel.cells = malloc(sizeof(selCol));
            sel.cells[0].row = tab->length - 1;
            sel.cells[0].col = 0;
            arow(tab,&sel);
            free(sel.cells);
        }
    }
    if(c >= tab->rows[0].length - 1){ //Přidání sloupců
        alignCols(tab,c);
    }
    cell.row = r;
    cell.col = c;
    return cell;
}

void selectWholeTab(Table *tab, Selection *sel){  
    sel->length = tab->length * tab->rows[0].length;
    //free(sel->cells);
    //sel->cells = malloc( sizeof(selCol) * sel->length ); 
    selCol *np = realloc(sel->cells,sizeof(selCol) * sel->length);
    if(np == NULL){
        perror("Error");
        sel->length=-1;
        return;
    }
    sel->cells = np;
    long cnt = 0;
    for(long r = 0; r < tab->length; r++){
        for(long c = 0; c < tab->rows[r].length;c++){
            sel->cells[cnt].row = r;
            sel->cells[cnt].col = c;
            cnt++;
        }
    }
}

void selectionGetNums(char *command, long *n1, long *n2){ //Z příkazu ve formátu [R,C] získá čísla R a C
    delChars(command,0,1); //Smaže [
    char *comma = strchr(command,','); //Ukazatel na , 
    char num1[comma - command + 1];
    strncpy(num1,command,comma- command); //do char num zkopíruje číslo
    num1[comma - command] = '\0'; //v num1 je R (str)
    delChars(command,0,comma - command + 1);
    char num2[strlen(command) - strlen(num1) + 1];
    strncpy(num2,command,strlen(command) - 1);
    num2[strlen(command) - 1] = '\0'; //v num2 je C (str)

    if(num1[0] != '_' ){
        char *endP;
        long val = strtol(num1,&endP,10);
        if(val < 1){ 
            *n1 = -1;
            return;
        }
        *n1 = val - 1;
    }else{ // R je _ -> značí výběr ze všech řádků
        *n1 = SELECTALL;
    }
    if(num2[0] != '_' ){
        char *endP;
        long val = strtol(num2,&endP,10);
        if(val < 1){
            *n2 = -1;
            return;
        }
        *n2 = val - 1;
    }else{ // S je _ -> výběr ze všech sloupců
        *n2 = SELECTALL;
    } // V n1,n2 jsou uloženy R a C
}

void min(Table *tab, Selection *sel){// [min] ve výběru najde buňku s nejmenší číselnou hodnotou
    double minVal = DOUBLEMAXVAL;
    selCol minLoc;
    for(long i = 0; i < sel->length; i++){
        char *dEnd;
        double val;
        if(tab->rows[ sel->cells[i].row ].cols[ sel->cells[i].col ].chars[0] == '"'){ //Pokud buňka začíná uvozovkou
            char valStr[tab->rows[ sel->cells[i].row ].cols[ sel->cells[i].col ].length + 1];
            strncpy(valStr,tab->rows[ sel->cells[i].row ].cols[ sel->cells[i].col ].chars,tab->rows[ sel->cells[i].row ].cols[ sel->cells[i].col ].length);
            delChars(valStr,0,1); //Smaže uvozovku před číslem
            val = strtod(valStr,&dEnd);
        }else{
            val = strtod(tab->rows[ sel->cells[i].row ].cols[ sel->cells[i].col ].chars,&dEnd);
        }
        bool isNumber = dEnd == tab->rows[ sel->cells[i].row ].cols[ sel->cells[i].col ].chars ? false : true;
        if(isNumber && val < minVal){
            minVal = val;
            minLoc = sel->cells[i];
        }
    }
    if(minVal != DOUBLEMAXVAL){ //Bylo nalezeno číslo 
        sel->length = 1;
        sel->cells[0] = minLoc;
        selCol *np = realloc(sel->cells,sizeof(selCol));
        if(np == NULL){
            sel->length = -1;
            return;
        }
        sel->cells = np;
    }

}

void max(Table *tab, Selection *sel){ // [max] ve výběru najde buňku s největší číselnou hodnotou
    float maxVal = -DOUBLEMAXVAL;
    selCol maxLoc;
    for(int i = 0; i < sel->length; i++){ //projde akutální selekci
        char *dEnd;
        double val;
        if(tab->rows[ sel->cells[i].row ].cols[ sel->cells[i].col ].chars[0] == '"'){ //Pokud buňka začíná uvozovkou
            char valStr[tab->rows[ sel->cells[i].row ].cols[ sel->cells[i].col ].length + 1];
            strncpy(valStr,tab->rows[ sel->cells[i].row ].cols[ sel->cells[i].col ].chars,tab->rows[ sel->cells[i].row ].cols[ sel->cells[i].col ].length);
            delChars(valStr,0,1); //Smaže uvozovku před číslem
            val = strtod(valStr,&dEnd);
        }else{
            val = strtod(tab->rows[ sel->cells[i].row ].cols[ sel->cells[i].col ].chars,&dEnd);
        }
        bool isNumber = dEnd == tab->rows[ sel->cells[i].row ].cols[ sel->cells[i].col ].chars ? false : true;
        if(isNumber && (val > maxVal || i == 0)){
            maxVal = val;
            maxLoc = sel->cells[i];
        }
    }
    if(maxVal != -DOUBLEMAXVAL){ //Bylo nalezeno číslo 
        sel->length = 1;
        sel->cells[0] = maxLoc;
        selCol *np = realloc(sel->cells,sizeof(selCol));
        if(np == NULL){
            sel->length = -1;
            return;}
        sel->cells = np;
    }
}

void find(Table *tab, Selection *sel,char *command){ //[find STR] - najde první buňku s podřetězcem STR
    int spaceLoc = strchr(command,' ') - command;
    delChars(command,0,spaceLoc + 1); //odstraní find 
    delChars(command,strlen(command) - 1, 1); //odstraní ]
    //v command je nyní už jen STR
    for(int i = 0; i < sel->length; i++){
        char *cellStr;
        if(strchr(tab->rows[ sel->cells[i].row ].cols[ sel->cells[i].col ].chars, '\\') != NULL){ //V buňce je zpětné lomítko -> pro správné porovnávání je ho nutné vymazat
            int backSLoc = strchr(tab->rows[ sel->cells[i].row ].cols[ sel->cells[i].col ].chars,'\\') - tab->rows[ sel->cells[i].row ].cols[ sel->cells[i].col ].chars; //Pozice zpětného lomítka
            cellStr = strdup(tab->rows[ sel->cells[i].row ].cols[ sel->cells[i].col ].chars);
            delChars(cellStr,backSLoc,1); //Smaže zpětné lomítko 
        }else{
            cellStr = strdup(tab->rows[ sel->cells[i].row ].cols[ sel->cells[i].col ].chars); //Obsah buňky k porovnání
        }
        if(strstr(cellStr,command) != NULL){ //i-tá buňka v selekci obsahuje STR
            sel->length = 1;
            sel->cells[0] = sel->cells[i]; //Uložení i-té buňky do selekce
            selCol *np = realloc(sel->cells,sizeof(selCol)); //případné zmenšení alokované paměti pro selekci
            if(np == NULL){
                sel->length = -1;
                return;}
            sel->cells = np;
            return;
        }
    }
}

void setVar(Table *tab, Selection *sel, Selection *selVar){ //funkce [set] = uložení do dočasné proměnné
     selCol *np = realloc(selVar->cells,sel->length * sizeof(selCol));
    if(np == NULL){
        perror("Error");
        selVar->length = -1;
        return;
    }
    selVar->cells = np;
    selVar->length = sel->length;
    for(long long i = 0; i < sel->length; i++){
        selVar->cells[i] = sel->cells[i];
    }
    if(tab->length){}; //(nepěkné)obejití erroru z Werror, mohl bych změnit argumenty funkce, potom bych ale musel v main kontrolovat a volat každou funkci zvlášť, místo jejich seskupení podle typu
}

void getVar(Table *tab, Selection *sel, Selection *selVar){// funkce [_] = obnovení výběru z dočasné proměnné
    if(selVar->length > 0){
        sel->length = selVar->length;
        selCol *np = realloc(sel->cells,selVar->length * sizeof(selCol));
        if(np == NULL){
            sel->length = -1;
            return;
        }
        sel->cells = np;
        for(long long i = 0; i < selVar->length;i++){
            sel->cells[i] = selVar->cells[i];
        }
    }
    if(tab->length){}; //Viz předchozí funkce
}

//Funkce pro úpravu obsahu buněk :

bool set(Table *tab, Selection *sel, char *command, char *del){ //Do buněk ve výběru nastaví STR

    delChars(command,0,4); //v command zůstane jen STR
    char str[(strlen(command) + 1) * 2 * sizeof(char)]; //Tak velké pole protože řetězec může být i jen x oddělovačů, v takovém případě by se jeho délka po escapování zdvojila
    str[0] = '\0';
    strcpy(str,command);
    long delPos;
    for(int i = 0; del[i] != '\0';i++){ //Kontrola jestli v STR je delimiter
        if(strchr(command,del[i]) != NULL){ //V STR je delimiter => je nutné ho escapovat pomocí zpětného lomítka
            delPos = strchr(command,del[i]) - command; //pozice oddělovače v STR
            if(command[delPos - 1] != '\\'){ //Přidat zpětné lomítko, jen pokud ho tam nevložil sám uživatel
                char newCommand[strlen(command) + 2];
                strncpy(newCommand,command,delPos);
                newCommand[delPos] = '\\';
                newCommand[delPos + 1] = '\0';
                delChars(command,0,delPos);
                strcat(newCommand,command);
                newCommand[strlen(command) + delPos + 2] = '\0';
                str[0] = '\0';
                strcpy(str,newCommand);
            }
        }
    }

    
    for(long i = 0; i < sel->length; i++){
        tab->rows[sel->cells[i].row].cols[sel->cells[i].col].length = strlen(str);
        free(tab->rows[sel->cells[i].row].cols[sel->cells[i].col].chars);
        tab->rows[sel->cells[i].row].cols[sel->cells[i].col].chars= malloc((strlen(str) + 1) * sizeof(char));
        strcpy(tab->rows[sel->cells[i].row].cols[sel->cells[i].col].chars,str);
    }
    return true;
}

bool clear(Table *tab, Selection *sel, char *command){ //Vymaže obsah vybraných buněk
    for(int i = 0; i < sel->length; i++){
        tab->rows[sel->cells[i].row].cols[sel->cells[i].col].length = 0;
        char *np = realloc(tab->rows[sel->cells[i].row].cols[sel->cells[i].col].chars, sizeof(char));
        if(np == NULL) return false;
        tab->rows[sel->cells[i].row].cols[sel->cells[i].col].chars = np;
        tab->rows[sel->cells[i].row].cols[sel->cells[i].col].chars[0] = '\0'; 
    }
    if(command){};
    return true;
}

bool contentGetNums(char *str, long *r, long *c){ //Pomocná funkce pro osamostatnění čísel řádku a sloupce z [R,C]
    delChars(str,0,1);
    long commaLoc = strchr(str,',') - str;
    char rStr[commaLoc];
    strncpy(rStr,str,commaLoc);
    rStr[commaLoc] = '\0';
    delChars(str,0,commaLoc + 1);
    char cStr[strlen(str) - 1];
    strncpy(cStr,str,strlen(str) - 1);
    cStr[strlen(str) - 1] = '\0';
    char *endP;
    *c = strtol(cStr,&endP,10) - 1;
    *r = strtol(rStr,&endP,10) - 1;
    return true;
}

bool swap(Table *tab, Selection *sel, char *command){ //vymění obsah vybrané buňky s buňkou na řádku R a sloupci C
    if(strchr(command,',') == 0 || strchr(command,'[') == 0 || strchr(command,']') == 0){ //Adresa buňky pro výměnu není platná
        return false;
    }
    long r,c;
    delChars(command,0,5); //Smaže "swap "
    contentGetNums(command,&r,&c); //Získá čísla sloupce a řádku
    if(r < 0 || r >= tab->length) return false;
    if(c < 0 || c >= tab->rows[0].length) return false;

    for(long i = 0; i < sel->length; i++){ //Nastaví buňku ve výběru na buňku ze swap a naopak
        char *valHolder = strdup(tab->rows[sel->cells[i].row].cols[sel->cells[i].col].chars); //Uložení hodnoty buňky ze selekce
        tab->rows[sel->cells[i].row].cols[sel->cells[i].col].length = tab->rows[r].cols[c].length; 
        char *np = realloc(tab->rows[sel->cells[i].row].cols[sel->cells[i].col].chars,(tab->rows[r].cols[c].length + 1) * sizeof(char));
        if(np == NULL) return false;
        tab->rows[sel->cells[i].row].cols[sel->cells[i].col].chars = np;
        strcpy(tab->rows[sel->cells[i].row].cols[sel->cells[i].col].chars,tab->rows[r].cols[c].chars); //Přepíše buňku ze selekce, buňkou určenou v swap 

        tab->rows[r].cols[c].length = strlen(valHolder);
        np = realloc(tab->rows[r].cols[c].chars,(strlen(valHolder) + 1) * sizeof(char));
        if(np == NULL) return false;
        tab->rows[r].cols[c].chars = np;
        strcpy(tab->rows[r].cols[c].chars,valHolder); //Přepíše buňku určenou swap, buňkou ze selekce
        free(valHolder);
    }

    return true;
}

bool sum(Table *tab, Selection *sel,char *command){ //do buňky na řádku R a sloupci C uloží součet hodnot vybraných buněk (odpovídající formátu %g u printf). Vybrané buňky neobsahující číslo budou ignorovány (jako by vybrány nebyly).
    delChars(command,0,4); //Smaže sum 
    if(strchr(command,',') == 0 || strchr(command,'[') == 0 || strchr(command,']') == 0){ //Adresa buňky pro výměnu není platná
        return false;
    }
    long r,c;
    contentGetNums(command,&r,&c); //načte čísla buněk
    if(r < 0 || r >= tab->length) return false;
    if(c < 0 || c >= tab->rows[0].length) return false;
    float sum = 0;
    for(long i = 0; i < sel->length; i++){ 
        sum += atof(tab->rows[sel->cells[i].row].cols[sel->cells[i].col].chars); //Sečte hodnoty buněk výběru
    }
    char sumStr[20];
    sprintf(sumStr,"%g",sum); //Zformátuje a uloží součet 
    tab->rows[r].cols[c].length = strlen(sumStr);
    char *np = realloc(tab->rows[r].cols[c].chars,(strlen(sumStr)+1) * sizeof(char)); 
    if(np == NULL) return false;
    tab->rows[r].cols[c].chars = np;
    strcpy(tab->rows[r].cols[c].chars,sumStr); //Zapíše součet do buňky R,C

    return true;
}

bool avg(Table *tab, Selection *sel, char *command){ // do buňky na řádku R a sloupci C se ukládá aritmetický průměr z vybraných buněk
    delChars(command,0,4); //Smaže avg  
    if(strchr(command,',') == 0 || strchr(command,'[') == 0 || strchr(command,']') == 0){ //Adresa buňky pro výměnu není platná
        return false;
    }
    long r,c;
    contentGetNums(command,&r,&c); //načte čísla buněk
    if(r < 0 || r >= tab->length) return false;
    if(c < 0 || c >= tab->rows[0].length) return false;
    float sum = 0;
    long count = 0;
    for(long i = 0; i < sel->length; i++){ 
        sum += atof(tab->rows[sel->cells[i].row].cols[sel->cells[i].col].chars); //Sečte hodnoty buněk výběru
        if('0' <= tab->rows[sel->cells[i].row].cols[sel->cells[i].col].chars[0] && tab->rows[sel->cells[i].row].cols[sel->cells[i].col].chars[0] <= '9'){ //Pokud je v buňce číslo
            count++;    //Zvýší se počet sečtených hodnot
        }
    }
    if(sum != 0 && count != 0){
        float avg = sum / count;
        char avgStr[20];
        sprintf(avgStr,"%g",avg); //Zformátuje aritmetický průměr na řetězec
        char *np = (char *) realloc(tab->rows[r].cols[c].chars,(strlen(avgStr) + 1) * sizeof(char));
        if(np == NULL) return false;
        tab->rows[r].cols[c].chars = np;
        tab->rows[r].cols[c].length = strlen(avgStr);
        strcpy(tab->rows[r].cols[c].chars,avgStr); //Uloží do R,C
    }
    return true;
}

bool count(Table *tab, Selection *sel, char *command){ // ukládá se počet neprázdných buněk z vybraných buněk
    delChars(command,0,6);
    if(strchr(command,',') == 0 || strchr(command,'[') == 0 || strchr(command,']') == 0){ //Adresa buňky pro výměnu není platná
        return false;
    }
    long r,c;
    contentGetNums(command,&r,&c);
    if(r < 0 || r >= tab->length) return false;
    if(c < 0 || c >= tab->rows[0].length) return false;
    long long count = 0;
    for(int i = 0; i < sel->length; i++){
        if(tab->rows[sel->cells[i].row].cols[sel->cells[i].col].length != 0){
            count++;
        }
    }
    char countStr[356];
    sprintf(countStr,"%lld",count);
    char *np = (char *) realloc(tab->rows[r].cols[c].chars,(strlen(countStr) + 1) * sizeof(char));
    if(np == NULL) return false;
    tab->rows[r].cols[c].chars = np;
    tab->rows[r].cols[c].length = strlen(countStr);
    strcpy(tab->rows[r].cols[c].chars,countStr); //Uloží do R,C
    return true;
}

bool len(Table *tab, Selection *sel, char *command){ //do buňky na řádku R a sloupci C uloží délku řetězce aktuálně vybrané buňky (První)
    delChars(command,0,4);
    if(strchr(command,',') == 0 || strchr(command,'[') == 0 || strchr(command,']') == 0){ //Adresa buňky pro výměnu není platná
        return false;
    }
    long r,c;
    contentGetNums(command,&r,&c);
    if(r < 0 || r >= tab->length) return false;
    if(c < 0 || c >= tab->rows[0].length) return false;
    long len = tab->rows[sel->cells[0].row].cols[sel->cells[0].col].length;
    char lenStr[356];
    sprintf(lenStr,"%ld",len);
    tab->rows[r].cols[c].length = strlen(lenStr);
    char *np = realloc(tab->rows[r].cols[c].chars,(strlen(lenStr)+1)*sizeof(char));
    if(np==NULL) return false;
    tab->rows[r].cols[c].chars = np;
    strcpy(tab->rows[r].cols[c].chars,lenStr);
    return true;
}

//Funkce pro práci s dočasnými proměnnými :

bool def(Table *tab, Selection *sel,char *command,char **tempVars){ //hodnota aktuální buňky bude nastavena do dočasné proměnné X
    delChars(command,0,5); //Smaže "def _" => v command zůstane jen číslo proměnné
    int var = atoi(command);
    if(var < 0 || var > 9) return false; //Neplatné číslo proměnné
    char *np = (char *)realloc(tempVars[var],(tab->rows[sel->cells[0].row].cols[sel->cells[0].col].length + 1) * sizeof(char)); //alokuje pomaět dočasné proměnné
    if(np == NULL) return false;
    tempVars[var] = np;
    strcpy(tempVars[var],tab->rows[sel->cells[0].row].cols[sel->cells[0].col].chars); //Do proměnné uloží hodnotu vybrané buňky
    return true;
}

bool use(Table *tab, Selection *sel,char *command,char **tempVars){ //aktuální buňka bude nastavena na hodnotu z dočasné proměnné
    delChars(command,0,5); //smaže "use _" => v command zůstane číslo proměnné
    int var = atoi(command);
    if(var < 0 || var > 9) return false; //Neplatné číslo proměnné
    tab->rows[sel->cells[0].row].cols[sel->cells[0].col].length = strlen(tempVars[var]);
    char *np = realloc(tab->rows[sel->cells[0].row].cols[sel->cells[0].col].chars,(strlen(tempVars[var])+1) * sizeof(char)); //alokuje místo v tabulce
    if(np == NULL) return false;
    tab->rows[sel->cells[0].row].cols[sel->cells[0].col].chars = np;
    strcpy(tab->rows[sel->cells[0].row].cols[sel->cells[0].col].chars,tempVars[var]); //Vybraná buňka je nastavena na hodnotu z dočasné proměnné
    return true;
}

bool inc(Table *tab, Selection *sel,char *command,char **tempVars){ //numerická hodnota v dočasné proměnné bude zvětšena o 1. Pokud dočasná proměnná neobsahuje číslo, bude výsledná hodnota proměnné nastavená na 1.
    delChars(command,0,5); //smaže "use _" => v command zůstane číslo proměnné
    int var = atoi(command);
    if(var < 0 || var > 9) return false; //Neplatné číslo proměnné
    char valueStr[356];
    if(tempVars[var][0] == '"') delChars(tempVars[var],0,1); //Pokud obsah začíná uvozovkou => je uvozovka smazána (Pro správnou detekci čísla)
    if(strchr(tempVars[var],'.') != 0){ //V dočasné proměnné je desetinné číslo
        float value = atof(tempVars[var]); //Převede řetězec na float, pokud v řetězci není číslo je rovna 0
        sprintf(valueStr,"%g",value + 1);
    }else{
        long value = strtod(tempVars[var],NULL); //V proměnné je celé číslo
        sprintf(valueStr,"%ld",value + 1);
    }
    char *np = realloc(tempVars[var],(strlen(valueStr)+1)*sizeof(char));
    if(np == NULL) return false;
    tempVars[var] = np;
    strcpy(tempVars[var],valueStr); //Uloží zvětšenou hodnotu
    if(tab->length || sel->length){};
    return true;
}


void basicSelect(Table *tab,Selection *sel, int n1, int n2){ //Výběr ve fromátu [R,C], kde R i C můžou být _
    if(n1 != SELECTALL && n2 != SELECTALL){ //R i C jsou čísla -> bude vybrána pouze jedna buňka
        sel->length = 1;
        sel->cells[0] = selectCell(tab,n1,n2);
        return;
    }else if(n1 == SELECTALL && n2 == SELECTALL){ // R i C jsou _ -> bude vybrána celá tabulka
        selectWholeTab(tab,sel);
        return;
    }else if(n1 == SELECTALL){ //Výběr sloupce ze všech řádků
        sel->length = tab->length;
        selCol *np = realloc(sel->cells,(tab->length) * sizeof(selCol)); //Rozšíření paměti
        if(np == NULL){
            perror("Error");
            sel->length = -1;
            return;
        }else{
            sel->cells = np;
        }
        for(long r = 0; r < tab->length; r++){
            sel->cells[r] = selectCell(tab,r,n2); //Pro všechny řádky vybere buňku ve sloupci n2
        }
        return;
    }else if(n2 == SELECTALL){ //Výběr všech sloupců v řádku
        sel->length = tab->rows[0].length;
        selCol *np = realloc(sel->cells,tab->rows[n1].length * sizeof(selCol));
        if(np == NULL){
            perror("Error");
            sel->length = -1;
            return;
        }else{
            sel->cells = np;
        }
        for(long c = 0; c < tab->rows[n1].length;c++){
            sel->cells[c].row = n1;
            sel->cells[c].col = c;
        }
        return;
    } 

}

void selectWindowGetNums(Table *tab,char *command, long *n){

    char nums[4][strlen(command) - 2]; //Pole pro ukládání řetězců s čísly 
    delChars(command,0,1); //Smaže [
    for(int i = 0; i < 3; i++){
        int commaLoc = strchr(command,',') - command; //Pozice , 
        strncpy(nums[i],command,commaLoc);
        nums[i][commaLoc] = '\0'; //do nums[i] uloží číslo i-té číslo v command
        delChars(command,0,commaLoc + 1); //Smaže číslo a ,
    }
    strncpy(nums[3],command,strlen(command) - 1); //Poslední číslo
    nums[3][strlen(command) - 1] = '\0'; // v nums[3] je R2

    for(int i = 0; i < 4; i++){ //Převede řetězce na čísla a uloží
        if(nums[i][0] != '-'){
            char *endP;
            long val = strtol(nums[i],&endP,10);
            if(val < 1){
                n[i] = -1;
                return;
            }else{
                n[i] = val - 1;
            }
        }else{
            if(i == 2){
                n[i] = tab->length - 1; //R2 je - => výběr největšího řádku
            }else if(i == 3){
                n[i] = tab->rows[0].length - 1; //C2 je - => výběr největšího sloupce
            }else{
                n[i] = -1; //R1 nebo C1 je - => chyba
            }
        }
    }    
}

void makeSelection(char *command, Table *tab, Selection *sel,Selection *selVar){ //Příkazy pro změnu výběru + [set]
    if(command[0] != '[' || command[strlen(command) - 1] != ']'){ //Příkaz není selekce
        sel->length = -1;
        return;
    }
    int commaCnt = 0;  // Počet čárek v příkazu -> slouží k rozlišení typů
    for(long i = 0; command[i] != '\0'; i++){
        if(command[i] == ','){
            commaCnt++;
        }
    }
    if(commaCnt == 0 && sel->length != -1){
        if(strstr(command,"min") != NULL){ // jedná se o příkaz [min]
            min(tab,sel);
            return;
        }else if(strstr(command,"max") != NULL){ //příkaz [max]
            max(tab,sel);
            return;
        }else if(strstr(command,"find") != NULL){ // příkaz [find STR] najde první buňku,, jejíž hodnota obsahuje podřetězec STR.
            find(tab,sel,command);
            return;
        }else if(strstr(command,"set") != 0){ //Příkaz [set] -> uloží aktuální výběr do proměnné _
            setVar(tab,sel,selVar);
            return;
        }else if(strchr(command,'_')){ //Příkaz [_] -> obnovení výběru z dočasné proměnné _
            getVar(tab,sel,selVar);
            return;
        }else{ //Neznámý příkaz
            sel->length = -1;
            return;
        }

    }else if(commaCnt == 1){ //příkaz je formátu se dvěma čísly [R,C]
        long n1,n2;
        selectionGetNums(command,&n1,&n2); //Z řetězce [R,C] osamostatní čísla R a C
        if(n1 == -1 || n2 == -1){
            sel->length = -1;
            return;
        }
        basicSelect(tab,sel,n1,n2); //Vybere buňku R C
        return;
    }else if(commaCnt == 3){ //[R1,C1,R2,C2] - výběr okna
        long nums[4];
        selectWindowGetNums(tab,command,nums);
        for(int i = 0; i < 4; i++){ //Kontrola získaných hodnot
            if(nums[i] == -1){
                sel->length = -1;
                return;
            }
        }
        long newSelLen = (nums[2] + 1 - nums[0]) * (nums[3] + 1 - nums[1]);
        sel->length = newSelLen;
        sel->cells = realloc(sel->cells,newSelLen * sizeof(selCol)); //Kontroly
        long cnt = 0;
        for(long r = nums[0]; r <= nums[2]; r++){ // R1 <= r >= R2
            for(long c = nums[1]; c <= nums[3]; c++){ // C1 <= c >= C2
                sel->cells[cnt] = selectCell(tab,r,c);
                cnt++;
            }
        }
    }else{
        sel->length = -1;
    }
}




int main(int argc,char *argv[]){
    char del[22] = " "; 
    bool delChanged = false;
    int returnValue = 0;
    if(argc == 1){
        printf("Nebyl zadán dostatek argumentů. Pro zobrazení nápovědy spusttě program s argumentem -help\n");
        return 0;
    }else if(argc>1){  // Načtení oddělovače
        for(int i = 1;i<argc;i++){ 
            if(strcmp(argv[i],"-d") == 0){ //Uživatel chce určit oddělovač
                delChanged = true;
                strcpy(del,argv[i+1]);
            }
        }
    }


    if(argc>=2){
        if(strstr(argv[1],"help")){
            help();
            return 0;
        }

        Table table;
        loadTable(&table,argv[argc - 1],del); //do struktury Table načte ze souboru tabulku
        if(table.length == -1){ //délka -1 značí chybu
            freeTable(&table); //deallokuje poamět pro tabulku
            returnValue = 1;
        }
        alignCols(&table,0); //Zarovná všechny řádky na stejný počet sloupců


        
        int commPos = delChanged ? 3 : 1; //Pozice řetězce s příkazy v argv
        Commands comms =  parseCommands(argv[commPos]); //Rozdělí řetězec příkazů na jednotlivé příkazy

        if(comms.length > 0 && table.length > 0){ //Pokud byly zadané nějaké příkazy
            Selection sel; //Proměnná pro ukládání výběru
            sel.cells = malloc(sizeof(selCol));
            sel.length = 1;
            sel.cells[0].col = 0; //Počáteční výběr [1,1]
            sel.cells[0].row = 0;

            Selection selVar; //Dočasná proměnná _ - pro selekci buňek 
            selVar.cells = malloc(sizeof(selCol));
            selVar.length = 0;

            char *tempVars[10]; //Dočasné proměnné _0 až _9
            for(int i = 0; i < 10; i++){ //Inicializace dočasných proměnných
                tempVars[i] = malloc(sizeof(char));
                tempVars[i][0] = '\0';
            }
            bool commandWasExecuted = false;
            for(int comm = 0; comm < comms.length; comm++){ //Projde příkazy
                commandWasExecuted = false;
                if( strstr(comms.commands[comm],"goto")){
                    
                }else if(comms.commands[comm][0] == '['){ //Příkaz na pozici comm je selekce
                    //Selekci řeším odděleně od ostatních příkazů protože má specifickou syntaxi
                    makeSelection(comms.commands[comm],&table,&sel,&selVar);//Provede selekci 
                    commandWasExecuted = true;
                    if(sel.length == -1){ //Nastala chyba
                        printf("Error : invalid argument for selection.\n");
                        returnValue = 1;
                        goto end;
                    }
                }else if(strstr(comms.commands[comm],"set") != 0){ //Set jako jediná z funkcí pro úpravu obsahu potřebuje předat i oddělovač, proto je osamostněná
                    commandWasExecuted = true;
                    if(set(&table,&sel,comms.commands[comm],del) == false){
                        fprintf(stderr,"Error : Something went wrong in set.\n");
                        returnValue = 1;
                        goto end;
                    }
                }else{
                    char *structureList[6] = {"irow","arow","drow","icol","acol","dcol"}; //Názvy funkcí, které upravují strukturu tabulky
                    int structureCount = 6;
                    bool (*structureFuncs[6])(Table *tab, Selection *sel) = {irow,arow,drow,icol,acol,dcol}; //Ukazatele na funkce pro úpravu struktury tabulky
                    for(int s = 0; s < structureCount; s++){ //Projde všechny příkazy pro úpravu struktury
                        if(strcmp(comms.commands[comm],structureList[s])==0){ //Porovná s aktuální příkazem
                            commandWasExecuted = true;
                            bool returnFlag = (*structureFuncs[s])(&table,&sel); //Provede příkaz na pozici comm
                            if(returnFlag == false){
                                fprintf(stderr,"Error : Something went wrong in %s.\n",structureList[s]);
                                returnValue = 1;
                                goto end;
                            }
                        }
                    }

                    char *contentList[6] = {"clear","swap","sum","avg","count","len"}; //Názvy funkcí, které upravují obsah buňky (buněk)
                    int contentCount = 6;
                    bool (*contentFuncs[6])(Table *tab, Selection *sel, char *command) = {clear,swap,sum,avg,count,len}; //Ukazatele na funkce pro úpravu obsahu buněk
                    for(int c = 0; c < contentCount; c++){ //Projde všechny příkazy pro úpravu struktury
                        if(strstr(comms.commands[comm],contentList[c])!=0){ //Porovná s aktuální příkazem
                            commandWasExecuted = true;
                            bool returnFlag = (*contentFuncs[c])(&table,&sel,comms.commands[comm]); //Provede příkaz na pozici comm
                            if(returnFlag == false){
                                fprintf(stderr,"Error : Something went wrong in %s.\n",contentList[c]);
                                returnValue = 1;
                                goto end;
                            }
                        }
                    }

                    char *varModificationList[3] = {"def","use","inc"};
                    int varModificationCount = 3;
                    bool (*varModificationFuncs[3])(Table *tab, Selection *sel,char *command, char **tempVars) = {def,use,inc};
                    for(int m = 0; m < varModificationCount; m++){
                        if(strstr(comms.commands[comm],varModificationList[m])!=0){
                            commandWasExecuted = true;
                            bool returnFlag = (*varModificationFuncs[m])(&table,&sel,comms.commands[comm],tempVars);
                            if(returnFlag == false){
                                fprintf(stderr,"Error : Something went wrong in %s.\n",varModificationList[m]);
                                returnValue = 1;
                                goto end;

                            }
                        }
                    }
                }
                if(commandWasExecuted == false){ //Příkaz nebyl rozpoznán
                    fprintf(stderr,"Error : Unkown command %s.\n",comms.commands[comm]);
                    returnValue = 1;
                    goto end;
                }
                /*
                printf("Selekce :\n");
                for(int i = 0; i < sel.length;i++){
                    printf("r %ld c %ld\n",sel.cells[i].row,sel.cells[i].col);
                }
                */
            }
            end:
            for(int i = 0; i < 10; i++){
                free(tempVars[i]);
            }
            free(sel.cells);
            free(selVar.cells);
            for(int i = 0; i < comms.length; i++){
                free(comms.commands[i]);
            }
            free(comms.commands);
        }
        
        

        if(table.length != -1 && returnValue != 1){
            printTable(&table,del,argv[argc - 1]); //Vytiskne/Zapíše do souboru tabulku 
            //DEBUG - má zapisovat do souboru
        }
        freeTable(&table); //Deallokace paměti pro tabulku

        
    }
    
    return returnValue;
}
