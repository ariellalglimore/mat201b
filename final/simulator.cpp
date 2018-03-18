#include "common.h"

#include "GLV/glv.h"
#include "alloGLV/al_ControlGLV.hpp"
#include "alloutil/al_AlloSphereAudioSpatializer.hpp"
#include "alloutil/al_Simulator.hpp"

const char* fileList[] = {"Australia.png",
                          "Cyprus.png",
                          "Serbia.png",
                          "New Zealand.png",
                          "Bosnia & Herzegovina.png",
                          "Argentina.png",
                          "Switzerland.png",
                          "Singapore.png",
                          "Hong Kong.png",
                          "Chile.png",
                          "Pakistan.png",
                          "Dominican Republic.png",
                          "United Arab Emirates.png",
                          "France.png",
                          "Malaysia.png",
                          "Croatia.png",
                          "Israel.png",
                          "South Africa.png",
                          "Belgium.png",
                          "Slovakia.png",
                          "Slovenia.png",
                          "Japan.png",
                          "Netherlands.png",
                          "Philippines.png",
                          "Colombia.png",
                          "Czechia.png",
                          "United Kingdom.png",
                          "United States.png",
                          "Bulgaria.png",
                          "Romania.png",
                          "Canada.png",
                          "India.png",
                          "Ireland.png",
                          "Spain.png",
                          "Thailand.png",
                          "Austria.png",
                          "Peru.png",
                          "Italy.png",
                          "Sweden.png",
                          "Greece.png",
                          "Mexico.png",
                          "Poland.png",
                          "Hungary.png",
                          "Taiwan.png",
                          "Germany.png",
                          "Finland.png",
                          "Denmark.png",
                          "Portugal.png",
                          "Vietnam.png",
                          "Venezuela.png",
                          "Indonesia.png",
                          "Morocco.png",
                          "Egypt.png",
                          "Russia.png",
                          "Norway.png",
                          "Ukraine.png",
                          "Brazil.png",
                          "Lithuania.png",
                          "Turkey.png",
                          "China.png",
                          "Saudi Arabia.png",
                          "Qatar.png",
                          "Uruguay.png",
                          "Costa Rica.png",
                          "Puerto Rico.png",
                          "South Korea.png",
                          "Ecuador.png",
                          "Iran.png",
                          "Latvia.png",
                          "Luxembourg.png",
                          "Tunisia.png",
                          "Estonia.png",
                          "Algeria.png",
                          "Kuwait.png",
                          "Kazakhstan.png",
                          "Montenegro.png",
                          "Macedonia.png",
                          "Belarus.png",
                          "Guatemala.png",
                          "Bolivia.png",
                          "Iraq.png",
                          "Nigeria.png",
                          "Honduras.png",
                          "Kenya.png",
                          "St. Helena.png",
                          "Paraguay.png"};

#define N (sizeof(fileList) / sizeof(fileList[0]))

struct MyApp : App, AlloSphereAudioSpatializer, InterfaceServerClient {
  vector<Vec3f> pos;
  Data data;

  GLVBinding gui;
  glv::Slider2D slider2d;
  glv::Slider scaleSlider;
  glv::Slider rateSlider;
  glv::Slider month;
  glv::Slider year;
  glv::Label monthlabel;
  glv::Label yearlabel;
  glv::Table layout;
  glv::Button labels;

  SoundSource aSoundSource;

  Texture texture[N];

  MyApp()
      : maker(Simulator::defaultBroadcastIP()),
        InterfaceServerClient(Simulator::defaultInterfaceServerIP()) {
    memset(state, 0, sizeof(State));

    for (int i = 0; i < N; i++) {
      Image image;
      if (!image.load(fullPathOrDie(fileList[i], ".."))) {
        cerr << "failed to load " << fileList[i] << endl;
        exit(1);
      }
      texture[i].allocate(image.array());
    }

    Image background;

    if (!background.load(fullPathOrDie("possiblebg.png"))) {
      fprintf(stderr, "FAIL\n");
      exit(1);
    }
    backTexture.allocate(background.array());

    addSphereWithTexcoords(backMesh, 1.3);
    backMesh.generateNormals();

    lens().far(1000);

    data.load(fullPathOrDie("finaltennisdata.csv"));

    addSphere(sphere);
    sphere.generateNormals();
    float worldradius = 1;
    for (int i = 0; i < data.row.size(); i++) {
      Vec3f position;
      position.x = -worldradius * cos((data.row[i].latitude) * (3.14 / 180)) *
                   cos((data.row[i].longitude) * (3.14 / 180));
      position.y = worldradius * sin((data.row[i].latitude) * (3.14 / 180));
      position.z = worldradius * cos((data.row[i].latitude) * (3.14 / 180)) *
                   sin((data.row[i].longitude) * (3.14 / 180));

      pos.push_back(position);
      for (int j = 0; j < data.row[0].monthData.size(); j++) {
        int outgoing =
            5 + (data.row[i].monthData[j] - 0) * (100 - 5) / (100 - 0);
        int outgoingcolor =
            0 + (data.row[i].colors[j] - 0) * (255 - 0) / (100 - 0);
        data.row[i].monthData[j] = outgoing;
        data.row[i].colors[j] = outgoingcolor;
      }
    }

    nav().pos().set(0, 0, 4);
    // nav().quat(Quatd(0.96, 0.00, 0.29, 0.00));
    nav().quat(Quatd(0, 0.00, 0, 0.00));
    initWindow();

    gui.bindTo(window());
    gui.style().color.set(glv::Color(0.7), 0.5);
    layout.arrangement(">p");

    scaleSlider.setValue(0);
    scaleSlider.interval(0, 0.007);
    layout << scaleSlider;
    layout << new glv::Label("scale");

    rateSlider.setValue(1);
    rateSlider.interval(.1, 8);
    layout << rateSlider;
    layout << new glv::Label("rate");

    month.setValue(0);
    month.interval(0, 11);
    layout << month;
    layout << monthlabel.setValue(months[0]);

    year.setValue(0);
    year.interval(0, 9);
    layout << year;
    layout << yearlabel.setValue(years[0]);

    layout << labels;
    layout.arrange();
    gui << layout;

    // audio
    AlloSphereAudioSpatializer::initAudio();
    AlloSphereAudioSpatializer::initSpatialization();
    // if gamma
    // gam::Sync::master().spu(AlloSphereAudioSpatializer::audioIO().fps());
    scene()->addSource(aSoundSource);
    scene()->usePerSampleProcessing(false);
  }

  cuttlebone::Maker<State> maker;
  State* state = new State;
  virtual void onAnimate(double dt) {
    while (InterfaceServerClient::oscRecv().recv())
      ;  // XXX

    state->angle += rateSlider.getValue();
    state->pose = nav();

    static double timer = 0;
    timer += dt;
    if (timer > 3) {
      timer -= 3;
      state->indexOfDataSet++;
      if (state->indexOfDataSet >= data.row[0].monthData.size())
        state->indexOfDataSet = 0;
    }

    state->course = -scaleSlider.getValue();
    state->turnOnLabels = labels.getValue();
    state->pose = nav();
    maker.set(*state);
  }

  Material material;
  Light light;
  Mesh sphere;
  virtual void onDraw(Graphics& g, const Viewpoint& v) {
    // cout << labels.getValue() << endl;
    backTexture.bind();
    g.draw(backMesh);
    backTexture.unbind();

    g.rotate(state->angle, 0, 1, 0);
    material();
    light();

    for (int i = 0; i < data.row.size(); i++) {
      g.pushMatrix();
      for (int j = 0; j < data.row[0].monthData.size(); j++) {
        g.color(HSV(data.row[i].colors[j] / 255.0, .4, .5));
      }
      g.translate(pos[i] + pos[i] *
                               data.row[i].monthData[state->indexOfDataSet] *
                               state->course);
      double scale = .001;
      g.scale(data.row[i].monthData[state->indexOfDataSet] * scale);
      g.draw(sphere);
      if (labels.getValue() == 1) {
        g.pushMatrix();
        g.translate(.9, 0, .9);
        texture[i].quad(g);
        g.popMatrix();
      }
      g.popMatrix();
    }
  }

  virtual void onSound(AudioIOData& io) {
    aSoundSource.pose(nav());
    while (io()) {
      aSoundSource.writeSample(0);
    }
    listener()->pose(nav());
    scene()->render(io);
  }
};

int main() {
  MyApp app;
  app.AlloSphereAudioSpatializer::audioIO().start();
  app.InterfaceServerClient::connect();
  app.maker.start();
  app.start();
}
