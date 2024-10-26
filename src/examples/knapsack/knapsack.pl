% Generate valid subsequences whose sum does not exceed Target
valid_subseq([], [], 0, _).
valid_subseq([H|T], [H|Subseq], Sum, Target) :-
    valid_subseq(T, Subseq, RestSum, Target),
    Sum is RestSum + H,
    Sum =< Target.
valid_subseq([_|T], Subseq, Sum, Target) :-
    valid_subseq(T, Subseq, Sum, Target).

% Find the best solution that maximises the sum but is =< Target
knapsack(Solution, SolutionSum, Files, Target) :-
    aggregate_all(max(SS, SL), valid_subseq(Files, SL, SS, Target), max(SolutionSum, Solution)).


/*
% Example query
?- knapsack(L, S, [1305892864, 1385113088, 856397968, 1106152425, 1647145093,
  1309917696, 1096825032, 1179242496, 1347631104, 696451130,
  746787826, 1080588288, 1165223499, 1181095818, 749898444, 1147613713,
  1280205208, 1242816512, 1189588992, 1232630196, 1291995024,
  911702020, 1678225920, 1252273456, 934001123, 863237392, 1358666176,
  1714134790, 1131848814, 1399329280, 1006665732, 1198348288,
  1090000441, 716904448, 677744640, 1067359748, 1646347388, 1266026326,
  1401106432, 1310275584, 1093615634, 1371899904, 736188416,
  1421438976, 1385125391, 1324463502, 1489042122, 1178813212,
  1239236096, 1258202316, 1364644352, 557194146, 555102962, 1383525888,
  710164700, 997808128, 1447622656, 1202085740, 694063104, 1753882504,
  1408100352], 8547993600).
*/