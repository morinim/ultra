% Solves the 8-Queens problem, where 8 queens must be placed on a chessboard so
% that no two queens attack each other.

% takeout/3 removes an element X from a list.
% Many Prolog libraries already have the delete/3 predicate.
takeout(X, [X|R], R).
takeout(X, [F|R], [F|S]) :- takeout(X, R, S).

% safe/1 checks if a list of queens is in a safe configuration.
safe([]).
safe([Queen|Others]) :-
    safe(Others),
    no_attack(Queen, Others, 1).

% no_attack/3 ensures that a queen does not attack others diagonally.
no_attack(_, [], _).
no_attack(Y, [Y1|Ylist], Xdist) :-
    abs(Y1-Y) =\= Xdist,  % ensure no diagonal conflict
    Dist1 is Xdist + 1,   % move to the next column distance
    no_attack(Y, Ylist, Dist1).

% place_queens/3: Places queens one by one, ensuring safety at each step.
place_queens([], PlacedQueens, PlacedQueens).  % all queens placed safely
place_queens(AvailableQueens, PlacedQueens, Result) :-
    member(Queen, AvailableQueens),  % choose a valid row for the queen
    safe([Queen|PlacedQueens]),      % ensure it's a safe placement
    takeout(Queen, AvailableQueens, RemainingQueens),
    place_queens(RemainingQueens, [Queen | PlacedQueens], Result).

solution(Queens) :-
    place_queens([1,2,3,4,5,6,7,8], [], Queens).

/*
% Example query
?- solution(X).
*/
