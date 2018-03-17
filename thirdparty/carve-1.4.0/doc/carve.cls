\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{carve}

\DeclareOption*{\PassOptionsToClass{\CurrentOption}{memoir}}
\ProcessOptions\relax
\LoadClass[12pt,a4paper,twoside,openright]{memoir}

% Extra packages
\usepackage{setspace}
\usepackage{multicol}
\usepackage{multirow}
\usepackage{ifthen}
\usepackage{calc}
\usepackage{listings}
\usepackage{color}
\usepackage{graphicx}
\usepackage{pgf}

\usepackage{fontspec,xltxtra,xunicode}

\definecolor{codebg}{cmyk}{0.09,0.06,0.00,0.00}       % E3E7F5
\definecolor{codeframe}{cmyk}{1.00,0.57,0.00,0.40}    % E3E7F5
\definecolor{codekwd}{cmyk}{1.00,0.57,0.00,0.40}      % 00447c
\definecolor{codecomment}{cmyk}{0.81,0.58,0.35,0.13}  % 3F607C
\definecolor{headingcolour}{cmyk}{1.00,0.57,0.00,0.40}% 00447c
\definecolor{headingtext}{gray}{1.0}

\defaultfontfeatures{Mapping=tex-text,Fractions=Off,Scale=MatchLowercase}

\settypeblocksize{237mm}{427pt}{*}
\setlrmargins{*}{*}{2}
\setulmargins{*}{*}{2}

\checkandfixthelayout
\typeoutlayout
\typeoutstandardlayout

\setromanfont[
  ItalicFont={Frutiger LT Std 46 Light Italic},
  BoldFont={Frutiger LT Std 55 Roman},
  BoldItalicFont={Frutiger LT Std 56 Italic},
]{Frutiger LT Std 45 Light}

\setsansfont[
  ItalicFont={Frutiger LT Std 46 Light Italic},
  BoldFont={Frutiger LT Std 55 Roman},
  BoldItalicFont={Frutiger LT Std 56 Italic},
]{Frutiger LT Std 45 Light}

\setmonofont[
  ItalicFont={Letter Gothic Std Slanted},
  BoldFont={Letter Gothic Std Bold},
  BoldItalicFont={Letter Gothic Std Bold Slanted},
]{Letter Gothic Std Medium}

\newfontfamily\condensedfont[
  ItalicFont={Frutiger LT Std 47 Light Condensed},
  BoldFont={Frutiger LT Std 57 Condensed},
  BoldItalicFont={Frutiger LT Std 57 Condensed},
]{Frutiger LT Std 47 Light Condensed}

\newfontfamily\boldcondensedfont[
  ItalicFont={Frutiger LT Std 67 Bold Condensed},
  BoldFont={Frutiger LT Std 77 Black Condensed},
  BoldItalicFont={Frutiger LT Std 77 Black Condensed},
]{Frutiger LT Std 67 Bold Condensed}

\newfontfamily\headingfont[
  ItalicFont={Frutiger LT Std 66 Bold Italic},
]{Frutiger LT Std 65 Bold}

\newfontfamily\codefont[]{Bitstream Vera Sans Mono}
\newfontfamily\codekwdfont[]{Bitstream Vera Sans Mono Bold}
\newfontfamily\codecommentfont[]{Bitstream Vera Sans Mono Oblique}

\lstset{frame=tblr,
        rulecolor=\color{codeframe},
        basicstyle=\codefont\small,
        keywordstyle=\codekwdfont\color{codekwd},
        commentstyle=\codecommentfont\color{codecomment},
        columns=fixed,
        captionpos=b,
        xleftmargin=2ex,xrightmargin=2ex,
        framexleftmargin=2ex,framexrightmargin=2ex
}

\newcommand\code[1]{{\codefont\mbox{#1}}}

\newsavebox{\sb@titlegfx}
\begin{lrbox}{\sb@titlegfx}\makebox[0mm][l]{\includegraphics{title-page.pdf}}\end{lrbox}

\usepackage[absolute,overlay]{textpos}
\textblockorigin{-6.33mm}{-1.14mm}
\setlength{\TPHorizModule}{210mm}
\setlength{\TPVertModule}{297mm}
\renewcommand\maketitle{\begin{textblock}{1}(0,0)\usebox\sb@titlegfx\end{textblock}~\clearpage}

\newlength{\leftmar}
\newlength{\@pad}
\newsavebox{\sb@chapnum}
\newsavebox{\sb@chaptitle}
\newsavebox{\sb@title}
\newcommand{\checkmargin}{
\strictpagechecktrue\checkoddpage
\ifoddpage\setlength{\leftmar}{1.0\spinemargin}\else\setlength{\leftmar}{1.0\foremargin}\fi
}
\newif\ifNoChapNumber
\makeatletter
\makechapterstyle{VZ34}{
\renewcommand\chapternamenum{}
\renewcommand\printchaptername{}
\renewcommand\printchapternum{}
\renewcommand\chapnumfont{\Large}
\renewcommand\chaptitlefont{\headingfont\Large}
\renewcommand\printchaptertitle[1]{%
\begingroup%
\setlength{\parindent}{0pt}
\checkmargin%
\color{headingtext}%
\begin{lrbox}{\sb@title}
\hspace{-1.0\leftmar}\begin{minipage}{210mm}
\colorbox{headingcolour}{\vspace{5pt}\begin{tabular}{@{}b{\leftmar-2ex}@{\vrule width 1pt}b{210mm+2ex-\leftmar}}
\chapnumfont \raggedleft \ifNoChapNumber \relax \else \thechapter. \fi \hspace{1ex} &
\chaptitlefont \raggedright \hspace{.5ex} ##1
\end{tabular}\vspace{2pt}}
\end{minipage}
\end{lrbox}
\usebox\sb@title
\NoChapNumberfalse
\endgroup%
}
\renewcommand\printchapternonum{\NoChapNumbertrue}
}
\chapterstyle{VZ34}
