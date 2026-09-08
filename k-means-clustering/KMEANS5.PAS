program KMeans5;

uses
  Crt, Dos;  {Crt for KeyPressed and Delay, Dos for Registers/intr}

const
  N = 1500;
  K = 6;

type
  TPoint = record
    X, Y: integer;
  end;

var
  Points: array[0..N-1] of TPoint;
  Pc: array[0..N-1] of byte;  {cluster index 0..5}
  Cent: array[0..K-1] of TPoint;
  Palette: array[0..K-1] of byte;
  I, C: integer;

{ Screen geometry for 320x200 mode }
const
  SCREEN_W = 320;
  SCREEN_H = 200;
  PAGE_BYTES = 80 * 200;  {80 bytes per scanline in mode 4}

{----------------------------------}
{ Low-level CGA plotting for 320x200 4-color mode (mode 4) }
{ Each byte holds 4 pixels (2 bpp). Segment B800:0000 }
{----------------------------------}
procedure PutPixel(X, Y: integer; Color: byte);
var
  p: ^byte;
  offset: word;
  shift: integer;
  mask: byte;
  cur: byte;
begin
  if (X < 0) or (X >= SCREEN_W) or (Y < 0) or (Y >= SCREEN_H) then
    Exit;

  offset := Y * 80 + (X shr 2);
  p := Ptr($B800, offset);
  shift := (3 - (X and 3)) * 2;  {0, 2, 4, 6}
  mask := not (3 shl shift);
  cur := p^;
  cur := (cur and mask) or (Color shl shift);
  p^ := cur;
end;

{ Clear the graphics page (fill with color 0) }
procedure ClearScreen;
begin
  FillChar(Ptr($B800, 0)^, PAGE_BYTES, 0);
end;

{------------------------------------------}
{ Distance squared (integer arithmetic) }
{------------------------------------------}
function Dist2(var A, B: TPoint): longint;
var
  dx, dy: longint;
begin
  dx := A.X - B.X;
  dy := A.Y - B.Y;
  Dist2 := dx * dx + dy * dy;
end;

{-----------------------------------------}
{ Assign each point to the nearest centroid }
{-----------------------------------------}
procedure AssignClusters;
var
  i, c: integer;
  bestC: integer;
  bestD, d: longint;
begin
  for i := 0 to N-1 do
  begin
    bestD := MaxInt;
    bestC := 0;
    for c := 0 to K-1 do
    begin
      d := Dist2(Points[i], Cent[c]);
      if d < bestD then
      begin
        bestD := d;
        bestC := c;
      end;
    end;
    Pc[i] := bestC;
  end;
end;

{---------------------------------------}
{ Recompute centroids (integer sums) }
{---------------------------------------}
function UpdateCentroids: boolean;
var
  c, i, Count: integer;
  SumX, SumY: longint;
  Old: TPoint;
  Changed: boolean;
begin
  Changed := False;
  for c := 0 to K-1 do
  begin
    Old := Cent[c];
    SumX := 0; SumY := 0; Count := 0;
    for i := 0 to N-1 do
      if Pc[i] = c then
      begin
        SumX := SumX + Points[i].X;
        SumY := SumY + Points[i].Y;
        Count := Count + 1;
      end;
    if Count > 0 then
    begin
      Cent[c].X := SumX div Count;
      Cent[c].Y := SumY div Count;
    end;
    if (Cent[c].X <> Old.X) or (Cent[c].Y <> Old.Y) then
      Changed := True;
  end;
  UpdateCentroids := Changed;
end;

{---------------------------------------------}
{ Random point generators }
{ Two options: rectangular uniform or polar cluster-like spread. }
{---------------------------------------------}
procedure RandomRect(var P: TPoint);
begin
  P.X := Random(SCREEN_W);
  P.Y := Random(SCREEN_H);
end;

procedure PolarRandom(var P: TPoint);
var
  cx, cy, radius, d: integer;
  ang: real;
begin
  cx := SCREEN_W div 2;
  cy := SCREEN_H div 2;
  if cx < cy then radius := cx else radius := cy;
  d := Random(radius);   {0..radius-1}
  ang := Random * 2.0 * Pi; {Random returns real when used in expression}
  P.X := cx + Trunc(d * Cos(ang));
  P.Y := cy + Trunc(d * Sin(ang));
  if P.X < 0 then P.X := 0;
  if P.X >= SCREEN_W then P.X := SCREEN_W - 1;
  if P.Y < 0 then P.Y := 0;
  if P.Y >= SCREEN_H then P.Y := SCREEN_H - 1;
end;

{----------------------------------------}
{ Draw all points and centroids with a small animation delay. }
{----------------------------------------}
procedure DrawAll;
var
  i: integer;
begin
  ClearScreen;
  for i := 0 to N-1 do
    PutPixel(Points[i].X, Points[i].Y, Palette[Pc[i]]);
    { Draw centroids brighter (use color 3) and a small cross. }
  for i := 0 to K-1 do
  begin
    PutPixel(Cent[i].X, Cent[i].Y, 3);
    PutPixel(Cent[i].X-1, Cent[i].Y, 3);
    PutPixel(Cent[i].X+1, Cent[i].Y, 3);
    PutPixel(Cent[i].X, Cent[i].Y-1, 3);
    PutPixel(Cent[i].X, Cent[i].Y+1, 3);
  end;
  Delay(10);
end;

{----------------------------------------}
{ KMeans main loop with iteration animation. }
{----------------------------------------}
procedure KMeans;
var
  Changed: boolean;
  iter: integer;
begin
  iter := 0;
  repeat
    AssignClusters;
    Changed := UpdateCentroids;
    DrawAll;
    Inc(iter);
    { Safety break to avoid infinite loop }
    if iter > 100 then
      Changed := False;
  until not Changed;
end;

{---------------------------------------------}
{ Set video mode using DOS registers and intr }
{---------------------------------------------}
procedure SetVideoMode(mode: word);
var
  regs: Registers;
begin
  regs.AX := mode;
  intr($10, regs);
end;

{----------------------------------------}
{ Exit on particular key press }
{----------------------------------------}
function Stop: boolean;
begin
  Stop := False;
  If not KeyPressed then Exit;
  while KeyPressed do if ReadKey = 'Q' then Stop := True;
end;

{------------------------------------------}
{ Program entry }
{------------------------------------------}
var
  regs: Registers;
  usePolar: boolean;
begin
  Randomize;

  { Initialize palette mapping to CGA 2-bit colors (0..3) }
  Palette[0] := 1;
  Palette[1] := 2;
  Palette[2] := 3;
  Palette[3] := 1;
  Palette[4] := 2;
  Palette[5] := 3;

  { enter CGA 320x200 4-color mode (function 0x0004) }
  SetVideoMode($0004);

  { choose distribution type }
  usePolar := True;

  repeat
    { generate random points }
    if usePolar then
      for I := 0 to N-1 do
        PolarRandom(Points[I])
    else
      for I := 0 to N-1 do
        RandomRect(Points[I]);

    { initial random centroids }
    for C := 0 to K-1 do
      RandomRect(Cent[C]);

    { run K-means with animation }
    KMeans;
  until Stop;

  { return to text mode 80x25 (mode 3) }
  SetVideoMode($0003);
end.