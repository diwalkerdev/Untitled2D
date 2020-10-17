#include "entity/core.hpp"
#include "gameevents.h"
#include <SDL2/SDL.h>
#include <vector>

void detect_hard_collisions(float                                    dt,
                            float                                    dt_step,
                            int                                      loop_idx,
                            GameEvents const&                        game_events,
                            entity::Player&                          player,
                            std::vector<entity::EntityStatic> const& walls,
                            std::vector<SDL_FRect> const&            hard_boundaries,
                            bool&                                    collided)
{
    entity::Player player_copy = player;
    for (int wall_idx = 0;
         (wall_idx < walls.size()) && !collided;
         ++wall_idx)
    {
        auto& boundary = hard_boundaries[wall_idx];
        auto& entity   = walls[wall_idx];

        auto& X = player.e.X;

        {
            linalg::Vectorf<2> c, r, p;

            collided = entity::is_point_in_rect(player.e.X[0][0],
                                                player.e.X[0][1],
                                                boundary);

            if (!collided)
            {
                player_copy = player;
            }
            else
            {
                // printf("Collided at iteration %d\n", i);

                // Go back to a position where the player is hasn't collided with the object.
                player = player_copy;

                linalg::Matrixf<2, 1> ux{{1.f, 0.f}};
                linalg::Matrixf<2, 1> uy{{0.f, 1.f}};

                // TODO: LA, automatically return float from the dot product below (eliminate [0]).
                auto  uxdot = (T(ux) * ux)[0];
                auto  uydot = (T(uy) * uy)[0];
                float ex    = uxdot * (entity.r.w * 0.5);
                float ey    = uydot * (entity.r.h * 0.5);

                c = sdl_rect_center(player.e);
                r = sdl_rect_center(entity.r);

                auto d = T(c - r);

                float dx = (d * ux)[0];
                float dy = (d * uy)[0];

                if (dx > ex)
                {
                    dx = ex;
                }
                if (dx < -ex)
                {
                    dx = -ex;
                }

                if (dy > ey)
                {
                    dy = ey;
                }
                if (dy < -ey)
                {
                    dy = -ey;
                }

                p = r + (dx * ux) + (dy * uy);

                auto c_vec = c - p;

                // NOTE: Alot of the code below is  more complicated than it actually needs to be
                // handling rectangles that don't rotate.

                // c_norm is a vector between the two centroids. If we use for reflections then collisions
                // at the edge of the object to reflect outwards at the angle of c_norm and not perpendicular
                // which is what we want for collisions between two rectangles.
                bool x_edge;
                {

                    float x1 = std::abs(p[0] - player.e.X[0][0]);
                    float x2 = std::abs(p[0] - (player.e.X[0][0] + player.e.w));

                    float y1 = std::abs(p[1] - player.e.X[0][1]);
                    float y2 = std::abs(p[1] - (player.e.X[0][1] + player.e.h));

                    float xmin = std::min(x1, x2);
                    float ymin = std::min(y1, y2);

                    x_edge = (xmin < ymin);
                }

                {
                    // TODO write norm function.
                    auto c_norm = norm(c_vec);
                    auto v      = linalg::Vectorf<2>(player.e.X[1]);
                    auto v_norm = norm(v);

                    float pv_x;
                    float pv_y;

                    // TODO: must be better way to refactor this.
                    if (x_edge)
                    {
                        auto c_norm_x = ((T(ux) * c_norm))[0];
                        auto c_norm_y = 0.f;

                        // So like above we should get the x and y part of the
                        // velocity vector by taking the doc product with with
                        // the basis vectors,
                        auto v_x = (c_norm_x * (T(ux) * (v_norm)))[0];

                        // TODO: should compare restitutions and use the smallest one.
                        pv_x = v_x * player.e.restitution;
                        pv_y = uydot; // allows gliding.
                    }
                    else
                    {
                        auto c_norm_x = 0.f;
                        auto c_norm_y = ((T(uy) * c_norm))[0];

                        auto v_y = (c_norm_y * (T(uy) * v_norm))[0];

                        pv_x = uxdot; // allows gliding.
                        pv_y = v_y * player.e.restitution;
                    }

                    player.e.X[1][0] *= pv_x;
                    player.e.X[1][1] *= pv_y;

                    // Calculate the time remaining after the collision.
                    float dt_eval = dt - (dt_step * loop_idx);
                    player.e.update(dt_eval, game_events.player_movement);
                }
            } // if collision.
        } // collision block.
    } // game entity loop.
}