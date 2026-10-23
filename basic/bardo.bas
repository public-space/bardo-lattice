' ==========================================================================
' bardo -- bardo lattice (BASIC edition)
'
' Written for FreeBASIC (https://www.freebasic.net), which runs on Linux,
' Windows and DOS.
'
'   Build:   fbc bardo.bas
'   Run:     ./bardo              menu-driven browser
'            ./bardo print        print the grid and exit
'            ./bardo --txt        first run only: .txt files instead of .md
'            ./bardo --year 2025  another year's folder
'
' The first run creates a folder named for the year in the current
' folder, with one file per day. It reads and writes the same folder as
' the Python, C and Bash editions.
' ==========================================================================

#include once "file.bi"

Const MARKER = ".bardo"

Dim Shared WEEKDAYS(0 To 6) As String = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"}
Dim Shared MONTHS(0 To 11) As String = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"}

Dim Shared As Integer yr, ndays, offs, ncols, todayI, firstRun, created
Dim Shared As String ext, ydir, todayStr
Dim Shared dstr(0 To 365) As String
Dim Shared As Integer wd(0 To 365), words(0 To 365), mstart(0 To 11)

' ---------- calendar arithmetic ----------
Function IsLeap(ByVal y As Integer) As Integer
    Return ((y Mod 4 = 0) And (y Mod 100 <> 0)) Or (y Mod 400 = 0)
End Function

Function MonthLen(ByVal y As Integer, ByVal m As Integer) As Integer
    Select Case m
        Case 2
            If IsLeap(y) Then Return 29 Else Return 28
        Case 4, 6, 9, 11
            Return 30
        Case Else
            Return 31
    End Select
End Function

' Sakamoto's method, converted so Monday = 0
Function WeekdayMon(ByVal y As Integer, ByVal m As Integer, ByVal d As Integer) As Integer
    Dim t(0 To 11) As Integer = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4}
    If m < 3 Then y -= 1
    Dim w As Integer = (y + y \ 4 - y \ 100 + y \ 400 + t(m - 1) + d) Mod 7
    Return (w + 6) Mod 7
End Function

Function Pad(ByVal n As Integer, ByVal w As Integer) As String
    Return Right(String(w, "0") & n, w)
End Function

Function DayPath(ByVal i As Integer) As String
    Return ydir & "/" & dstr(i) & "." & ext
End Function

' ---------- build the year ----------
Sub BuildDays()
    Dim As Integer m, d
    ndays = 0
    For m = 1 To 12
        mstart(m - 1) = ndays
        For d = 1 To MonthLen(yr, m)
            dstr(ndays) = Pad(yr, 4) & "-" & Pad(m, 2) & "-" & Pad(d, 2)
            wd(ndays) = WeekdayMon(yr, m, d)
            ndays += 1
        Next
    Next
    offs = wd(0)
    ncols = (ndays + offs + 6) \ 7
    todayI = -1
    For d = 0 To ndays - 1
        If dstr(d) = todayStr Then todayI = d
    Next
End Sub

' ---------- first run: folder, marker, one node per day ----------
Sub InitYear(ByVal wantTxt As Integer)
    Dim As Integer f, i
    Dim As String ln, p
    ydir = CurDir() & "/" & yr
    If wantTxt Then ext = "txt" Else ext = "md"
    p = ydir & "/" & MARKER
    If FileExists(p) Then
        f = FreeFile
        If Open(p For Input As #f) = 0 Then
            Do While Not Eof(f)
                Line Input #f, ln
                If Left(ln, 7) = "format=" Then
                    If Mid(ln, 8, 3) = "txt" Then ext = "txt" Else ext = "md"
                End If
            Loop
            Close #f
        End If
    Else
        firstRun = -1
        MkDir ydir
        f = FreeFile
        Open p For Output As #f
        Print #f, "format=" & ext
        Print #f, "created=" & todayStr
        Print #f, "by=bardo.bas"
        Close #f
    End If
    For i = 0 To ndays - 1
        p = DayPath(i)
        If Not FileExists(p) Then
            f = FreeFile
            Open p For Output As #f
            If ext = "md" Then
                Print #f, "# " & WEEKDAYS(wd(i)) & ", " & dstr(i)
            Else
                Print #f, WEEKDAYS(wd(i)) & ", " & dstr(i)
            End If
            Print #f, ""
            Close #f
            created += 1
        End If
    Next
End Sub

' ---------- words after the header line ----------
Function CountWords(ByVal p As String) As Integer
    Dim As Integer f, n, i, inWord, pastHeader
    Dim As String ln, c
    f = FreeFile
    If Open(p For Input As #f) <> 0 Then Return 0
    Do While Not Eof(f)
        Line Input #f, ln
        If pastHeader = 0 Then
            pastHeader = -1
        Else
            inWord = 0
            For i = 1 To Len(ln)
                c = Mid(ln, i, 1)
                If c = " " Or c = Chr(9) Or c = Chr(13) Then
                    inWord = 0
                ElseIf inWord = 0 Then
                    inWord = -1
                    n += 1
                End If
            Next
        End If
    Loop
    Close #f
    Return n
End Function

Sub Scan()
    Dim i As Integer
    For i = 0 To ndays - 1
        words(i) = CountWords(DayPath(i))
    Next
End Sub

Function Glyph(ByVal i As Integer) As String
    If i = todayI Then Return "@"
    If words(i) > 0 Then
        If words(i) < 50 Then Return "+"
        If words(i) < 250 Then Return "*"
        Return "#"
    End If
    If dstr(i) > todayStr Then Return "."
    Return "o"
End Function

' color numbers: 14 yellow (today), 12 light red (written), 8 grey (not yet), 7 normal
Function GlyphColor(ByVal i As Integer) As Integer
    If i = todayI Then Return 14
    If words(i) > 0 Then Return 12
    If dstr(i) > todayStr Then Return 8
    Return 7
End Function

' ---------- the grid ----------
Sub DrawGrid(ByVal useColor As Integer)
    Dim As Integer row, col, i, m, x, lastEnd, written, total, passed, streak
    Dim As String labels = Space(ncols + 4)
    lastEnd = -1
    For m = 0 To 11
        x = (mstart(m) + offs) \ 7
        If x > lastEnd Then
            Mid(labels, x + 1, 3) = Left(MONTHS(m), 3)
            lastEnd = x + 3
        End If
    Next
    Print "bardo lattice  " & yr
    Print
    Print "    " & RTrim(labels)
    For row = 0 To 6
        If row Mod 2 = 0 Then
            Print Left(WEEKDAYS(row), 3) & " ";
        Else
            Print "    ";
        End If
        For col = 0 To ncols - 1
            i = col * 7 + row - offs
            If i < 0 Or i >= ndays Then
                Print " ";
            Else
                If useColor Then Color GlyphColor(i)
                Print Glyph(i);
                If useColor Then Color 7
            End If
        Next
        Print
    Next
    For i = 0 To ndays - 1
        If words(i) > 0 Then written += 1
        total += words(i)
        If dstr(i) <= todayStr Then passed += 1
    Next
    If todayI >= 0 Then
        i = todayI
        If words(i) = 0 Then i -= 1
        Do While i >= 0
            If words(i) = 0 Then Exit Do
            streak += 1
            i -= 1
        Loop
    End If
    Print
    Print "@ today   . not yet   o passed, empty   + under 50 words   * under 250   # 250 or more"
    Print yr & ": " & ndays & " days, " & passed & " passed, " & written & " written, " & total & " words, streak " & streak
    Print "folder: " & ydir & "  (." & ext & ")"
End Sub

' ---------- opening and reading days ----------
Sub OpenDay(ByVal i As Integer)
    Dim ed As String = Environ("VISUAL")
    If ed = "" Then ed = Environ("EDITOR")
    If ed = "" Then ed = "nano"
    Shell ed & " """ & DayPath(i) & """"
    words(i) = CountWords(DayPath(i))
End Sub

Sub ViewDay(ByVal i As Integer)
    Dim As Integer f, n
    Dim ln As String
    Cls
    Print WEEKDAYS(wd(i)) & ", " & MONTHS(Val(Mid(dstr(i), 6, 2)) - 1) & " " & Val(Mid(dstr(i), 9, 2)) & ", " & yr & "   (" & words(i) & " words)"
    Print String(60, "-")
    f = FreeFile
    If Open(DayPath(i) For Input As #f) = 0 Then
        If Not Eof(f) Then Line Input #f, ln   ' skip the header line
        Do While Not Eof(f)
            Line Input #f, ln
            Print ln
            n += 1
        Loop
        Close #f
    End If
    If words(i) = 0 Then Print "(nothing written in this day yet)"
    Print String(60, "-")
    Print "Press Enter to go back.";
    Line Input ln
End Sub

Sub ListWritten()
    Dim As Integer i, n
    Dim ln As String
    Cls
    Print "Days you have written in, " & yr
    Print
    For i = 0 To ndays - 1
        If words(i) > 0 Then
            Print "  #" & Pad(i + 1, 3) & "  " & dstr(i) & "  " & Left(WEEKDAYS(wd(i)), 3) & "  " & words(i) & " words"
            n += 1
        End If
    Next
    If n = 0 Then Print "  None yet. Open today with T and write something."
    Print
    Print "Press Enter to go back.";
    Line Input ln
End Sub

Function FindDate(ByVal s As String) As Integer
    Dim i As Integer
    For i = 0 To ndays - 1
        If dstr(i) = s Then Return i
    Next
    Return -1
End Function

' ---------- main ----------
Dim As Integer wantTxt, printOnly, k, i, n
Dim As String a, cmd, arg, msg
Dim dt As String = Date              ' FreeBASIC gives mm-dd-yyyy
todayStr = Mid(dt, 7, 4) & "-" & Left(dt, 2) & "-" & Mid(dt, 4, 2)
yr = Val(Mid(dt, 7, 4))

k = 1
Do While Command(k) <> ""
    a = Command(k)
    If a = "--txt" Then
        wantTxt = -1
    ElseIf a = "print" Then
        printOnly = -1
    ElseIf a = "--year" Then
        k += 1
        yr = Val(Command(k))
    End If
    k += 1
Loop

BuildDays()
InitYear(wantTxt)
Scan()

If printOnly Then
    If firstRun Then Print "First run: created " & ydir & " with " & created & " day files (." & ext & ")." : Print
    DrawGrid(0)
    End
End If

If firstRun Then msg = "First run: created " & yr & "/ with " & created & " day files (." & ext & ")."

Do
    Cls
    DrawGrid(-1)
    Print
    If msg <> "" Then Print msg : Print : msg = ""
    Print "  T          open today in your editor"
    Print "  D date     open a date            D " & todayStr
    Print "  N number   open day number 1-" & ndays & "     N 72"
    Print "  V date     read a day here        V " & todayStr
    Print "  L          list the days you have written in"
    Print "  R          rescan the folder"
    Print "  Q          quit"
    Print
    Line Input "> ", cmd
    cmd = Trim(cmd)
    arg = Trim(Mid(cmd, 2))
    Select Case UCase(Left(cmd, 1))
        Case "Q"
            Exit Do
        Case "T"
            If todayI >= 0 Then OpenDay(todayI) Else msg = "Today is not in " & yr & "."
        Case "D", "V"
            i = FindDate(arg)
            If i < 0 Then
                msg = "Not a day in " & yr & ": '" & arg & "'. Use YYYY-MM-DD."
            ElseIf UCase(Left(cmd, 1)) = "D" Then
                OpenDay(i)
            Else
                ViewDay(i)
            End If
        Case "N"
            n = Val(arg)
            If n >= 1 And n <= ndays Then OpenDay(n - 1) Else msg = "Day number must be 1 to " & ndays & "."
        Case "L"
            ListWritten()
        Case "R"
            Scan()
            msg = "Rescanned the folder."
        Case ""
        Case Else
            msg = "Unknown command: " & cmd
    End Select
Loop
Color 7
