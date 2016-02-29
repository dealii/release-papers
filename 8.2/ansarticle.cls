% $Id: ansarticle.cls 2730 2011-06-03 09:53:18Z logg $

% FIXME: (AL) Need to choose a license for this style, I suggest GFDL

% Basic setup
\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{ansarticle}
\LoadClass[twoside,a4paper]{article}

% Load packages
\RequirePackage{a4wide}
\RequirePackage{anslistings}
\RequirePackage{algorithm,algorithmicx,algpseudocode}
\RequirePackage{amsmath,amssymb}
\RequirePackage[noblocks]{authblk}
\RequirePackage{booktabs}
\RequirePackage{fancyhdr}
\RequirePackage{graphicx}
\RequirePackage[hyperfootnotes=false,colorlinks,linkcolor=blue,urlcolor=blue,citecolor=blue]{hyperref}
\RequirePackage{natbib}

\RequirePackage{lastpage}
\RequirePackage{listings}
\RequirePackage[colorinlistoftodos,textwidth=4cm,shadow]{todonotes}


% Font selection
\RequirePackage[T1]{fontenc}
\RequirePackage{ae,aecompl}
\RequirePackage{pxfonts}        % Palatino for running text and math
\renewcommand{\sfdefault}{pag}  % Avantgarde for sans serif (section headings)

% Section headings
\renewcommand{\section}{%
  \@startsection{section}{0}{0pt}{2ex}{1ex}{\large\bf\sffamily}}
\renewcommand{\subsection}{%
  \@startsection{subsection}{1}{0pt}{\baselineskip}{.5ex}{\bf\sffamily}}
\renewcommand{\subsubsection}{%
  \@startsection{subsubsection}{2}{0pt}{\baselineskip}{-1em}{\bf\sffamily}}
\renewcommand{\paragraph}{%
  \@startsection{paragraph}{3}{0pt}{0ex}{-0.5em}{\bf\sffamily}}
\renewcommand{\subparagraph}{%
  \@startsection{subparagraph}{4}{0pt}{0ex}{-0.5em}{\bf\sffamily}}
\setcounter{secnumdepth}{10}

% Paragraphs
\setlength{\parindent}{0pt}
\setlength{\parskip}{1ex plus 2ex}

% Running title and author
\let\@runningtitle\@empty
\let\@runningauthor\@empty
\newcommand{\runningtitle}[1]{\gdef\@runningtitle{#1}}
\newcommand{\runningauthor}[1]{\gdef\@runningauthor{#1}}

% Formatting volume and article number
\newread\datafile
\openin\datafile=ansinfo
\ifeof\datafile
\message{File datafile.tex with volume, number, and year is missing}
\def\ANSreceived{never}
\def\ANSrevised{never}
\def\ANSaccepted{never}
\def\ANSpublished{never}
\def\ANSvolume{VV}
\def\ANSnumber{NN}
\def\ANSyear{1677}
\else
\closein\datafile
\input{ansinfo}
\fi

% Page headers and footers
\fancyhead[RO, LE]{\thepage}
\fancyhead[LO]{\@runningtitle}
\fancyhead[RE]{\@runningauthor}
\fancyfoot[RE,LO]{%
  \footnotesize \copyright{} by the authors, \ANSyear}
\fancyfoot[RO,LE]{%
  \footnotesize Archive of Numerical Software \ANSvolume(\ANSnumber), \ANSyear}
\fancyfoot[C]{}
\renewcommand{\headrulewidth}{0pt}
\pagestyle{fancy}

% Front matter
\renewcommand*{\title}[2][]{\gdef\s@title{#1}\gdef\@title{#2}}
\renewcommand{\Authfont}{\large\sffamily}
\renewcommand{\Affilfont}{\normalsize\sffamily}
\renewcommand{\maketitle}{
  % Footnote stuff
  \let\footnotesize\small
  \def\footnoterule{\vspace*{-3pt}\rule{.4\columnwidth}{.4pt}\vspace*{2.6pt}}
  \renewcommand\thefootnote{\sffamily\@fnsymbol\c@footnote}
  \let\footnote\thanks
  % Title
  \hfill\fboxrule2pt\fboxsep3mm\fbox{\parbox{5cm}{\centering
        Archive of Numerical Software\\
        vol. \ANSvolume, no. \ANSnumber, pages 1--\pageref*{LastPage}\\
        \copyright{} \ANSyear}}
  \vskip 60\p@
  \begin{center}
    {\LARGE \textsf{\textbf{\@title}} \par}
    \vskip 8mm
    {\large \lineskip .75em \textsf{\@author} }
    \vskip 4mm
  \end{center}\par
  % Affiliation stuff
  \@thanks
  % Paper received
  \vskip 3mm
  \textbf{\sffamily Received: }\ANSreceived;
  \textbf{\sffamily final revision:} \ANSrevised;
  \textbf{\sffamily published:} \ANSpublished.
  % Some more fixes
  \vskip 3mm
  \setcounter{footnote}{0}%
  \global\let\thanks\relax
  \global\let\maketitle\relax
  \global\let\@thanks\@empty
  \global\let\@author\@empty
  \global\let\@date\@empty
  \global\let\@title\@empty
  \global\let\title\relax
  \global\let\author\relax
  \global\let\date\relax
  \global\let\and\relax
  % No headers/footers on front page
  \thispagestyle{empty}
}

% Abstract heading
\renewenvironment{abstract}{\textsf{\textbf{Abstract: }}}{}

% Useful macro
\newcommand{\fixme}[1]{\todo[inline, color=red!40]{#1}}

% Some useful notation macros
\newcommand{\R}{\mathbb{R}}
\newcommand{\dx}{\,\mathrm{d}x}
\newcommand{\dX}{\,\mathrm{d}X}
\newcommand{\ds}{\,\mathrm{d}s}
\newcommand{\dS}{\,\mathrm{d}S}
\newcommand{\dt}{\,\mathrm{d}t}
\newcommand{\inner}[2]{\langle #1, #2 \rangle}
\newcommand{\nedelec}{N\'ed\'elec}
\newcommand{\babuska}{Babu\v{s}ka}
\newcommand{\emp}[1]{\texttt{#1}}
