#ifndef NETWORK_HPP_
#define NETWORK_HPP_

#include <Layer.hpp>
#include "FMap.hpp"

#include <boost/numeric/ublas/matrix_sparse.hpp>
#include <boost/numeric/ublas/vector_sparse.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <fstream>
#include <cmath>

namespace ffnn
{
    using namespace boost::numeric::ublas;

    template<typename T>
    class Network
    {
    public:
        typedef std::vector<Layer<T>> layer_list;

        //! Push back a layer on the layer list
        //! if the input size match the previous layer
        //! output size (number of neurons).
        bool connect_layer(const Layer<T> &layer)
        {
            if (!layer.valid())
                return false;
            if (!layers.empty())
                if (layers.back().get_output_size() != layer.get_input_size())
                    return false;

            layers.push_back(layer);
            return true;
        }

        //! Remove the last layer on the layer list.
        void disconnect_layer()
        {layers.pop_back();};

        //! Return the list of layers
        const layer_list& get_layers()
        {return layers;};

        //! Compute forward pass of the network
        //! \return The list of neuron outputs. The input is saw as the first layer.
        std::vector<vector<T>> forward(const vector<T> &input)
        {
            std::vector<vector<T>> out_list;

            out_list.push_back(input);
            for (auto layer : layers)
                out_list.push_back(layer << out_list.back());

            return out_list;
        }

        //! Evaluate a network
        vector<T> eval(const vector<T> &input)
        {
            vector<T> output(input);
            for (auto layer : layers)
                output = output >> layer;
            return output;
        }

        void train(T h, const vector<T> &input, const vector<T> &output)
        {
            //////////////////////////////////////////
            // Compute the forward pass from the input
            //
            auto a_vec = forward(input);

            /////////////
            // Compute the delta_list, wich is the list of all derivative
            // dC_over_dz where z_l is the weightenen sum of an input incoming
            // into the layer l.

            // L means the last layer, and l a layer beetween 1(input) and L.
            // dC_over_da means the gradient of C on the direction a.
            auto dC_over_da = a_vec.back() - output;

            // The delta list is the list of all gradients in reverse order
            std::vector<vector<T>> delta_list;

            // Reverse order browsing of outputs of neurons
            auto a_vec_it = a_vec.rbegin();
            //Delta L :
            vector<T> delta_L = element_prod(dC_over_da,
                                                    layers.back().derivative_function % *a_vec_it);
            delta_list.push_back(delta_L);
            a_vec_it++;
            for (auto l : boost::adaptors::reverse(layers))
            {
                auto exp1 = prod(trans(l.weights), delta_list.back());
                auto exp2 = l.derivative_function % *a_vec_it;
                delta_list.push_back(element_prod(exp1, exp2));
                //Notice we are also computing the derivative of C over
                //the input, wich could be used to extract 'images patchs'.
                a_vec_it++;
            };

            ///////////////////////////////////////////////////////////////
            // Compute the derivative of the weights and the biases, namely
            // dC_over_dw and dC_over_db = delta_list and apply the modification
            // to the layer. They are stored in reverse order.
            //auto &dC_over_db = delta_list;

            // Reverse order (right -> left) browsing of a_vec and delta_list.
            // We throw the last vector of a_vec and the first vector of delta_list.
            a_vec_it = ++a_vec.rbegin();
            auto delta_it = delta_list.begin();
            for (auto &l : boost::adaptors::reverse(layers))
            {
                auto m = outer_prod(*delta_it, *a_vec_it);

                // Apply L1 regularisation
                const T s = (T)0.00001;
                auto f = [&](T x) -> T
                {
                    T r = std::abs(x) - h*s;
                    if (r <= 0)
                        return 0;
                    else
                        return std::copysign(r, x);
                };
                f %= l.weights;
                f %= l.biases;

                // Update with the gradient
                l.weights -= h * m;
                l.biases -= h * (*delta_it);

                a_vec_it++;
                delta_it++;
            }
        }

        friend
        std::ostream &operator<< (std::ostream &oss, const Network<T> &n)
        {
            boost::property_tree::write_json(oss, n.serialize());
            return oss;
        }

        boost::property_tree::ptree serialize() const
        {
            boost::property_tree::ptree root;

            for (auto l : layers)
                root.add_child("network.layers.layer", l.serialize());

            return root;
        }

        void save_file(std::string filename)
        {
            std::ofstream ofs(filename);
            boost::property_tree::write_json(ofs, serialize());
            ofs << "\n";
        }

        bool load(const boost::property_tree::ptree &tree)
        {
            layers.resize(0);

            try
            {
                for (auto &l : tree.get_child("network.layers"))
                {
                    Layer<T> layer;
                    if (!layer.load(l.second)
                        || !connect_layer(std::move(layer)))
                    {
                        layers.resize(0);
                        return false;
                    }
                }
            }
            catch(std::exception &e)
            {
#ifdef DEBUG
                std::cerr << e.what();
#endif
            }
            return true;
        }

        void load_file(std::string filename)
        {
            namespace pt = boost::property_tree;

            pt::ptree tree;

            std::ifstream ifs(filename);
            pt::read_json(ifs, tree);

            load(tree);
        }

    private:
        layer_list layers;
    };
}

#endif /* !NETWORK_HPP_ */
