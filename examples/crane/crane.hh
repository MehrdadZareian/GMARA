
#include <vector>
#define verbose true
#define pi 3.14

/* state space dim */
const int state_dim=5;
/* input space dim */
const int input_dim=1;
/*
 * data types for the state space elements and input space
 * elements used in uniform grid and ode solvers
 */
using state_type = std::array<double,state_dim>;
using input_type = std::array<double,input_dim>;

using abs_type = scots::abs_type;


/*Assigning values to parametrs*/
class Parameters {
  public:

    const char* address="nom_tr.txt";

    /* lower bounds of the hyper rectangle */
state_type s_lb={{-0.2,-2,1.2,-5.5,-0.1}};
    /* upper bounds of the hyper rectangle */
state_type s_ub={{5.5,4.4,4.7,5.3,7}};
    /* grid node distance diameter */
state_type s_eta={{0.015,0.035,0.016,0.064,0.1}};

    /* lower bounds of the hyper rectangle */
input_type i_lb={-7};
    /* upper bounds of the hyper rectangle */
input_type i_ub={7};
    /* grid node distance diameter */
input_type i_eta={{0.2}};

    /*Disturbance vector(for each state)*/
state_type w={{0,0.05,0,0,0}};

/*Growth bond matrix */
double L_first[71][16];
double L_second[71][16];

    /*sampling time*/
    const double tau =0.1;

    /*number of samples in nominal trajectory (trajectory is discrete time)*/
    int trajectory_size=70;
    /* tube size is equal to number of boxes times */
    state_type epsilon_num={8,10,10,11,0};

    const int state_dim=5;
    const int input_dim=1;
    const int trajectory_num=0; //Caution : starts from zero
    const int agent_num=2;
    std::vector<state_type> initial_trajectory_states{ {{0,0,pi,0,0}}
                                                     };


} parameters;





/* we integrate the growth bound by tau sec (the result is stored in r)  */
inline auto radius_post = [](state_type &r, const state_type &, const input_type &u) {
   state_type r_temp;
   static state_type w=parameters.w;
    for (int i=0;i<4;i++)
        r_temp[i]=0;

    int index=(u[0]+7)/0.2;
    for (int i=0;i<4;i++)
        for(int j=0;j<4;j++){
            r_temp[i]+= r[j]*parameters.L_first[index][4*j+i] + w[j]*parameters.L_second[index][4*j+i];
        }

    for (int i=0;i<4;i++)
        r[i]=r_temp[i];

    r[4]=0;
};







/* we integrate the unicycle ode by tau sec (the result is stored in x)  */
inline auto  ode_post = [](state_type &x, const input_type &u) {
    /* the ode describing the vehicle */
    double tau=parameters.tau;

    auto rhs =[](state_type& xx,  const state_type &x, const input_type &u) { // state space model


        double g = 9.8;
        double M_c=1.0;
        double M_p=0.1;
        double M_t=M_p+M_c;
        double l=0.5;
        double c1 = u[0]/M_t;
        double c2 = M_p*l/M_t;
        double c3=l*4.0/3.0;
        double c4=l*M_p/M_t;

        double F=(g*std::sin(x[2])-std::cos(x[2])*(c1+c2*x[3]*x[3]*std::sin(x[2])))/(c3-c4*std::cos(x[2])*std::cos(x[2]));
        double G= (c1+c2*x[3]*x[3]*std::sin(x[2])) - c4* std::cos(x[2])*F;

        xx[0] =x[1];
        xx[1] =G;
        xx[2] =x[3];
        xx[3] =F;
        xx[4]=1;


    };
    scots::runge_kutta_fixed4(rhs,x,u,state_dim,tau,1);

};


auto  ode_post_dist = [](state_type &x, const input_type &u) {
    /* the ode describing the vehicle */
    double tau=parameters.tau;
    static state_type w=parameters.w;
    auto rhs =[](state_type& xx,  const state_type &x, const input_type &u) { // state space model


        double g = 9.8;
        double M_c=1.0;
        double M_p=0.1;
        double M_t=M_p+M_c;
        double l=0.5;
        double c1 = u[0]/M_t;
        double c2 = M_p*l/M_t;
        double c3=l*4.0/3.0;
        double c4=l*M_p/M_t;

        double F=(g*std::sin(x[2])-std::cos(x[2])*(c1+c2*x[3]*x[3]*std::sin(x[2])))/(c3-c4*std::cos(x[2])*std::cos(x[2]));
        double G= (c1+c2*x[3]*x[3]*std::sin(x[2])) - c4* std::cos(x[2])*F;

        xx[0] =x[1]+w[0];
        xx[1] =G+w[1];
        xx[2] =x[3]+w[2];
        xx[3] =F+w[3];
        xx[4]=1;

    };
    scots::runge_kutta_fixed4(rhs,x,u,state_dim,tau,1);

};

std::vector<state_type> trajectory_simulation(Parameters& parameters){
    std::vector<state_type> nominal_trajectory_states;

    int trajectory_size=parameters.trajectory_size;
    int tr_num=parameters.trajectory_num;
    state_type initial_trajectory_state=parameters.initial_trajectory_states[tr_num];
    std::ifstream inFile;
    inFile.open(parameters.address);


    std::vector<input_type> u_temp(parameters.agent_num);
    std::vector<input_type> inputs(trajectory_size);

    for ( int i = 0; i <trajectory_size ; i++) {
        for (int k = 0; k < parameters.agent_num ; ++k) {
            for(int q=0;q<input_dim;q++)
                inFile >> u_temp[k][q];
        }
      inputs[i]=u_temp[tr_num];
    }

    std::ifstream inFile2;
    inFile2.open("GB.txt");

    for (int i =0 ; i<142;i++){
        for(int j=0;j<16;j++){
            if(i%2==0){
                inFile2 >> parameters.L_first[i/2][j];
            }
            else
                inFile2 >> parameters.L_second[i/2][j];

        }
    }
    state_type temp_state;
    nominal_trajectory_states.push_back(initial_trajectory_state);
    temp_state=initial_trajectory_state;
    state_type min_x=initial_trajectory_state;
    state_type max_x=initial_trajectory_state;


    for ( int i = 0; i <trajectory_size-1 ; i++)
    {
        ode_post (temp_state,inputs[i]);
        for (int i = 0; i < state_dim; ++i) {
            if(min_x[i]>temp_state[i])
                min_x[i]=temp_state[i];
            if(max_x[i]<temp_state[i])
                max_x[i]=temp_state[i];
        }

        nominal_trajectory_states.push_back(temp_state);
        if(verbose)
            std::cout<< i+2<<"- " << nominal_trajectory_states[i+1][0]<< "  " <<nominal_trajectory_states[i+1][1]<<"  " <<nominal_trajectory_states[i+1][2]<<"  " << std::endl;
    }
    if(verbose)
        for (int i = 0; i < state_dim; ++i)
            std::cout<<i<<" min:"<<min_x[i]<<" max:"<<max_x[i]<<std::endl;

    return nominal_trajectory_states;

}








