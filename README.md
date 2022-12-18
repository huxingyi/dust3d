# Dust3D

Dust3D is a cross-platform 3D modeling software that makes it easy to create low poly 3D models for video games, 3D printing, and more.

## Getting Started

These instructions will get you a copy of the Dust3D up and running on your local machine for development and testing purposes.

### Prerequisites

In order to build and run Dust3D, you will need the following software and tools:

- Qt
- Visual Studio 2019 (Windows only)
- GCC (Linux only)

#### Windows

1. Download and install the `Qt Online Installer`
2. Run the installer and choose the Qt archives you want to install (required: qtbase, qtsvg)
3. Install Visual Studio 2019

#### Linux

1. Install Qt using your distribution's package manager (e.g. `apt-get install qt5-default libqt5svg5-dev` on Ubuntu)

### Building

A step by step series of examples that tell you how to get a development environment running

1. Clone the repository
```
git clone https://github.com/huxingyi/dust3d.git
```

2. Build using qmake

#### Windows

1. Open the Qt Creator IDE
2. Select "Open Project" from the File menu
3. Navigate to the project directory `dust3d/application` and open the `.pro` file
4. Select the desired build configuration (e.g. Debug or Release) from the dropdown menu at the bottom left of the window
5. Click the "Run" button to build and run the project

#### Linux

1. Change to the project directory  `dust3d/application`
2. Run `qmake` to generate a Makefile
3. Build the project using `make`

## License

Dust3D is licensed under the MIT License - see the [LICENSE](https://github.com/huxingyi/dust3d/blob/master/LICENSE) file for details.

<!-- Sponsors begin -->
## Sponsors  

<a href="https://github.com/fohara" title="fohara" target=_blank><image src="https://avatars.githubusercontent.com/u/1181948?u=00445fb982564b2a4461c5848662ac37f1405b01&v=4" alt="@fohara" width="35" height="35" style="border-radius: 17px;" /></a>
<a href="https://github.com/kidandcat" title="kidandcat" target=_blank><image src="https://avatars.githubusercontent.com/u/3855826?u=64040c971ad9bf1e66dc5d4df54871d0af9c890f&v=4" alt="@kidandcat" width="35" height="35" style="border-radius: 17px;" /></a>
<a href="https://github.com/DisownedWheat" title="DisownedWheat" target=_blank><image src="https://avatars.githubusercontent.com/u/11870491?u=a8b24ff1daf2577b1afac771fd2cf80e39c8b4b0&v=4" alt="@DisownedWheat" width="35" height="35" style="border-radius: 17px;" /></a>
<a href="https://github.com/Nobuna-no" title="Nobuna-no" target=_blank><image src="https://avatars.githubusercontent.com/u/43406539?u=fc0d4b157b4b49775d415b2f405d5c45a486a107&v=4" alt="@Nobuna-no" width="35" height="35" style="border-radius: 17px;" /></a>
<a href="https://github.com/Ghee36" title="Ghee36" target=_blank><image src="https://avatars.githubusercontent.com/u/70682472?v=4" alt="@Ghee36" width="35" height="35" style="border-radius: 17px;" /></a>

 _The list shown represent active sponsors on GitHub and a full list can be viewed at [SUPPORTERS](https://github.com/huxingyi/dust3d/blob/master/SUPPORTERS)._
<!-- Sponsors end -->