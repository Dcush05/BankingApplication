       IDENTIFICATION DIVISION.
       PROGRAM-ID. ACCOUNT.
       AUTHOR. DYLAN CUSHNIE.
       ENVIRONMENT DIVISION.
       INPUT-OUTPUT SECTION.
       FILE-CONTROL.
       *> The master ledger which is our database that stores account data
       SELECT MASTER-FILE ASSIGN TO "data/master-ledger.dat"
           ORGANIZATION IS LINE SEQUENTIAL.

       *>now we are just working with one user(bankteller) 
       DATA DIVISION.
       FILE SECTION.
       FD MASTER-FILE.
       01 MASTER-RECORD.
           05 M-ACCT-ID PIC X(10).
           05 M-CUSTOMER-NAME PIC X(10).
           05 M-BALANCE PIC S9(10)V99.
          

       WORKING-STORAGE SECTION.
       01 WS-FLAGS.
           05 ACCOUNT-STATUS-FLAG PIC X VALUE 'N'.
              88 ACCOUNT-NOT-FOUND VALUE 'N'.
              88 ACCOUNT-FOUND VALUE 'Y'.
       01 WS-EOF-FLAG  PIC X VALUE 'N'.
           88 END-OF-FILE VALUE 'Y'.
       LINKAGE SECTION.
       01 DFHCOMMAREA.
           05 CA-ACCT-ID PIC X(10).
           05 CA-CUSTOMER-NAME PIC X(10).
           05 CA-TRANS-TYPE PIC X(10).
           05 CA-AMOUNT USAGE COMP-2.
           05 CA-RESULT-BA USAGE COMP-2.
           05 CA-STATUS  PIC S9(8) COMP-5.



       PROCEDURE DIVISION USING DFHCOMMAREA.
       MAINLINE.
           INITIALIZE WS-EOF-FLAG WS-FLAGS
           OPEN I-O MASTER-FILE
           PERFORM UNTIL END-OF-FILE OR ACCOUNT-FOUND
              READ MASTER-FILE
              AT END SET END-OF-FILE TO TRUE
              NOT AT END
                 IF M-ACCT-ID = CA-ACCT-ID
                    SET ACCOUNT-FOUND TO TRUE
                    END-IF
               END-READ
           END-PERFORM
           DISPLAY "We made it here"

       *>Note(Dylan)->TRIM THIS SO ITS A LOT CLEANER
           IF ACCOUNT-FOUND
              IF CA-TRANS-TYPE = "DEPOSIT   "
                 ADD CA-AMOUNT TO M-BALANCE
              ELSE
                 SUBTRACT CA-AMOUNT FROM M-BALANCE
              END-IF

              REWRITE MASTER-RECORD

              MOVE M-BALANCE TO CA-RESULT-BA
              MOVE 0 TO CA-STATUS
           ELSE
              MOVE 1 TO CA-STATUS
           END-IF
                    
           CLOSE MASTER-FILE
           GOBACK. 