# Wine recognition data
Last updated Sept 21, 1998 by C.Blake

## Sources

- Forina, M. et al, PARVUS - An Extendible Package for Data Exploration, Classification and Correlation. Institute of Pharmaceutical and Food Analysis and Technologies, Via Brigata Salerno, 16147 Genoa, Italy.
- Stefan Aeberhard, email: stefan@coral.cs.jcu.edu.au

## Past Usage

1. S. Aeberhard, D. Coomans and O. de Vel, Comparison of Classifiers in High Dimensional Settings, Tech. Rep. no. 92-02, (1992), Dept. of Computer Science and Dept. of Mathematics and Statistics, James Cook University of North Queensland (Also submitted to Technometrics).
   The data was used with many others for comparing various classifiers. The classes are separable, though only RDA has achieved 100% correct classification (RDA : 100%, QDA 99.4%, LDA 98.9%, 1NN 96.1% (z-transformed data)).
   In a classification context, this is a well posed problem with "well behaved" class structures. A good data set for first testing of a new classifier, but not very challenging.
2. S. Aeberhard, D. Coomans and O. de Vel, "THE CLASSIFICATION PERFORMANCE OF RDA" Tech. Rep. no. 92-01, (1992), Dept. of Computer Science and Dept. of Mathematics and Statistics, James Cook University of North Queensland (Also submitted to Journal of Chemometrics).

## Relevant Information

These data are the results of a chemical analysis of wines grown in the same region in Italy but derived from three different cultivars.
The analysis determined the quantities of 13 constituents found in each of the three types of wines.

I think that the initial data set had around 30 variables, but for some reason I only have the 13 dimensional version. I had a list of what the 30 or so variables were, but a.) I lost it, and b.), I would not know which 13 variables are included in the set.

The attributes are (dontated by Riccardo Leardi, riclea@anchem.unige.it):

1. Alcohol
2. Malic acid
3. Ash
4. Alcalinity of ash
5. Magnesium
6. Total phenols
7. Flavanoids
8. Nonflavanoid phenols
9. Proanthocyanins
10. Color intensity
11. Hue
12. OD280/OD315 of diluted wines
13. Proline

Number of instances:

- **class 1** 59
- **class 2** 71
- **class 3** 48

Number of attributes: 13

For Each Attribute:

- all attributes are continuous;
- no statistics available, but suggest to standardise variables for certain uses (e.g. for us with classifiers which are NOT scale invariant).

NOTE: 1st attribute is class identifier (1-3)

Missing Attribute Values: none.
